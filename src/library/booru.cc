#include <booru/booru.hh>

#include <booru/db/entities/post.hh>
#include <booru/db/entities/post_file.hh>
#include <booru/db/entities/post_site_id.hh>
#include <booru/db/entities/post_tag.hh>
#include <booru/db/entities/post_type.hh>
#include <booru/db/entities/site.hh>
#include <booru/db/entities/tag.hh>
#include <booru/db/entities/tag_implication.hh>
#include <booru/db/entities/tag_type.hh>
#include <booru/db/query.hh>
#include <booru/db/visitors.hh>
#include <booru/db/stmt.hh>
#include <booru/log.hh>
#include <booru/result.hh>

#include "db/sql.hh"
#include "db/sqlite3/db.hh"

#include <log4cxx/basicconfigurator.h>

namespace Booru
{

    Owning<Booru> Booru::InitializeLibrary()
    {
        return Owning<Booru>(new Booru);
    }

    int64_t Booru::GetSchemaVersion()
    {
        return SQLGetSchemaVersion();
    }

    Booru::Booru()
    {
        log4cxx::BasicConfigurator::resetConfiguration();
        log4cxx::BasicConfigurator::configure();
        log4cxx::Logger::getRootLogger()->setLevel(log4cxx::Level::getInfo());

        LOG_INFO("Booru library initialized");
    }

    Booru::~Booru()
    {
        LOG_INFO("Booru library shutting down");
        CloseDatabase();
    }

    ResultCode Booru::OpenDatabase(StringView const &_Path, bool _Create)
    {
        LOG_INFO("Opening database at '{}'...", _Path);

        CloseDatabase();

        auto db = DB::Sqlite3::DatabaseSqlite3::OpenDatabase(_Path);
        if (!db)
        {
            return db.Code;
        }

        // TODO: other databases

        m_DB = db.Value;

        DB::TransactionGuard guard(m_DB);

        LOG_INFO("Checking database version...");
        auto dbVersion = GetConfigInt64("db.version");
        if (!dbVersion)
        {
            if (!_Create)
            {
                guard.Rollback();
                m_DB = nullptr;
                return ResultCode::InvalidArgument;
            }

            CHECK_RETURN_RESULT_ON_ERROR(CreateTables(), "Table creation failed.");
            dbVersion = GetConfigInt64("db.version");
            CHECK_RETURN_RESULT_ON_ERROR(dbVersion.Code, "Missing database version even after table creation.");
        }

        LOG_INFO("Sucessfully opened database. Version {}", dbVersion.Value);

        for (DB::INTEGER i = dbVersion.Value; i < SQLGetSchemaVersion(); i++)
        {
            CHECK_RETURN_RESULT_ON_ERROR(UpdateTables(i));
        }

        guard.Commit();

        return ResultCode::OK;
    }

    void Booru::CloseDatabase()
    {
        if (m_DB)
        {
            while (m_DB->IsInTransaction())
                CHECK(m_DB->RollbackTransaction());
        }
        m_DB = nullptr;
    }

    DB::ExpectedDB Booru::GetDatabase()
    {
        if (m_DB)
            return {m_DB};
        return {ResultCode::InvalidState};
    }

    Expected<DB::TEXT> Booru::GetConfig(DB::TEXT const &_Name)
    {
        CHECK_VAR_RETURN_RESULT_ON_ERROR(db, GetDatabase());
        CHECK_VAR_RETURN_RESULT_ON_ERROR(stmt, DB::Query::Select("Config").Column("Value").Key("Name").Prepare(db.Value));
        CHECK_RETURN_RESULT_ON_ERROR(stmt.Value->BindValue("Name", _Name));
        return stmt.Value->ExecuteScalar<DB::TEXT>();
    }

    ResultCode Booru::SetConfig(DB::TEXT const &_Name, DB::TEXT const &_Value)
    {
        CHECK_VAR_RETURN_RESULT_ON_ERROR(db, GetDatabase());
        CHECK_VAR_RETURN_RESULT_ON_ERROR(stmt, DB::Query::Upsert("Config").Column("Value").Column("Name").Prepare(db.Value));
        CHECK_RETURN_RESULT_ON_ERROR(stmt.Value->BindValue("Name", _Name));
        CHECK_RETURN_RESULT_ON_ERROR(stmt.Value->BindValue("Value", _Value));
        CHECK_RETURN(stmt.Value->Step(false));
    }

    Expected<DB::INTEGER> Booru::GetConfigInt64(DB::TEXT const &_Name)
    {
        CHECK_VAR_RETURN_RESULT_ON_ERROR(config, GetConfig(_Name));
        // TODO: error handling
        return std::stoll(config);
    }

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // PostTypes
    // ////////////////////////////////////////////////////////////////////////////////////////////

    ExpectedVector<DB::Entities::PostType> Booru::GetPostTypes()
    {
        CHECK_VAR_RETURN_RESULT_ON_ERROR(db, GetDatabase());
        return DB::Entities::GetAll<DB::Entities::PostType>(db.Value);
    }

    Expected<DB::Entities::PostType> Booru::GetPostType(DB::INTEGER _Id)
    {
        CHECK_VAR_RETURN_RESULT_ON_ERROR(db, GetDatabase());
        return DB::Entities::GetWithKey<DB::Entities::PostType>(db.Value, "Id", _Id);
    }

    Expected<DB::Entities::PostType> Booru::GetPostType(DB::TEXT const &_Name)
    {
        CHECK_VAR_RETURN_RESULT_ON_ERROR(db, GetDatabase());
        return DB::Entities::GetWithKey<DB::Entities::PostType>(db.Value, "Name", _Name);
    }

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // Posts
    // ////////////////////////////////////////////////////////////////////////////////////////////

    ExpectedVector<DB::Entities::Post> Booru::GetPosts()
    {
        return GetDatabase()
            .Then(DB::Entities::GetAll<DB::Entities::Post>);
    }

    Expected<DB::Entities::Post> Booru::GetPost(DB::INTEGER _Id)
    {
        return GetDatabase()
            .Then([&](auto db)
                  { return DB::Entities::GetWithKey<DB::Entities::Post>(db, "Id", _Id); });
    }

    Expected<DB::Entities::Post> Booru::GetPost(DB::BLOB<16> _MD5)
    {
        return GetDatabase()
            .Then([&](auto db)
                  { return DB::Entities::GetWithKey<DB::Entities::Post>(db, "MD5Sum", _MD5); });
    }

    ResultCode Booru::AddTagToPost(DB::INTEGER _PostId, StringView const &_Tag)
    {
        StringView tagName = _Tag;
        bool remove = _Tag[0] == '-';
        if (remove)
        {
            tagName = tagName.substr(1);
        }

        // find tag
        CHECK_VAR_RETURN_RESULT_ON_ERROR(tag, GetTag(DB::TEXT(tagName)));

        if (remove)
            return RemoveTagFromPost(_PostId, tag.Value.Id);

        // follow redirections
        int iteration = 0;
        while (tag.Value.RedirectId.has_value())
        {
            // TODO: make this a setting
            if (iteration++ > 32)
            {
                return ResultCode::RecursionExceeded;
            }
            tag = GetTag(tag.Value.RedirectId.value());
            CHECK_RETURN_RESULT_ON_ERROR(tag);
        }

        CHECK_VAR_RETURN_RESULT_ON_ERROR(db, GetDatabase());
        DB::TransactionGuard transactionGuard(db);

        // actually add tag
        DB::Entities::PostTag postTag;
        postTag.PostId = _PostId;
        postTag.TagId = tag.Value.Id;

        CHECK_RETURN_RESULT_ON_ERROR(Create(postTag));

        // try to honor implications, ignore errors
        CHECK_VAR_RETURN_RESULT_ON_ERROR(implications, GetTagImplicationsForTag(tag.Value.Id));
        for (auto const &implication : implications.Value)
        {
            DB::INTEGER impliedTagId = implication.ImpliedTagId;
            if (implication.Flags & DB::Entities::TagImplication::FLAG_REMOVE_TAG)
            {
                // might fail if tag has not been added, that's fine, ignore
                CHECK(RemoveTagFromPost(_PostId, tag.Value.Id));
            }
            else
            {
                auto impliedTag = GetTag(impliedTagId);
                CHECK_CONTINUE_ON_ERROR(impliedTag);

                // recurse, added tag may have redirection, implications, ...
                // might fail if already added, ignore
                CHECK(AddTagToPost(_PostId, impliedTag.Value.Name.c_str()));
            }
        }

        transactionGuard.Commit();
        return ResultCode::OK;
    }

    ResultCode Booru::RemoveTagFromPost(DB::INTEGER _PostId, DB::INTEGER _TagId)
    {
        return FindPostTag(_PostId, _TagId)
            .Then([this](auto pt)
                  { return Delete(pt); });
    }

    ExpectedVector<DB::Entities::Post> Booru::FindPosts(StringView const &_QueryString)
    {
        StringVector queryStringTokens = Strings::Split(_QueryString);
        if (queryStringTokens.empty())
            return ResultCode::InvalidRequest;

        auto query =
            DB::Query::Select(DB::Entities::Post::Table);

        for (auto const &tag : queryStringTokens)
            query.Where(GetSQLConditionForTag(tag));

        return query
            .Prepare(m_DB)
            .Then([](auto s)
                  { return s->template ExecuteList<DB::Entities::Post>(); });
    }

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // PostTags
    // ////////////////////////////////////////////////////////////////////////////////////////////

    ExpectedVector<DB::Entities::PostTag> Booru::GetPostTags()
    {
        return GetDatabase()
            .Then(DB::Entities::GetAll<DB::Entities::PostTag>);
    }

    ExpectedVector<DB::Entities::Tag> Booru::GetTagsForPost(DB::INTEGER _PostId)
    {
        static auto tagIdSubQuery =
            DB::Query::Select(DB::Entities::PostTag::Table)
                .Column("TagId")
                .Where(DB::Query::Where::Equal("PostId", "$PostId "));

        static auto tagQuery =
            DB::Query::Select(DB::Entities::Tag::Table)
                .Where(DB::Query::Where::In("Tags.Id", tagIdSubQuery));

        return GetDatabase()
            .Then([&](auto db)
                  { return tagQuery.Prepare(db); })
            .Then([&](auto s)
                  { return s->BindValue("PostId", _PostId); })
            .Then([&](auto s)
                  { return s->template ExecuteList<DB::Entities::Tag>(); });
    }

    ExpectedVector<DB::Entities::Post> Booru::GetPostsForTag(DB::INTEGER _TagId)
    {
        static auto postIdSubQuery =
            DB::Query::Select(DB::Entities::PostTag::Table)
                .Column("TagId")
                .Where(DB::Query::Where::Equal("TagId", "$TagId "));

        static auto postQuery =
            DB::Query::Select(DB::Entities::Post::Table)
                .Where(DB::Query::Where::In("Posts.Id", postIdSubQuery));

        return GetDatabase()
            .Then([](auto db)
                  { return postQuery.Prepare(db); })
            .Then([&](auto s)
                  { return s->BindValue("TagId", _TagId); })
            .Then([](auto s)
                  { return s->template ExecuteList<DB::Entities::Post>(); });
    }

    template <class TValue>
    static inline auto BindValue(DB::StmtPtr _Stmt, StringView const &_Name, TValue const &_Value)
    {
        return _Stmt->BindValue(_Name, _Value);
    }

    Expected<DB::Entities::PostTag> Booru::FindPostTag(DB::INTEGER _PostId, DB::INTEGER _TagId)
    {
        static auto postTagQuery =
            DB::Query::Select(DB::Entities::PostTag::Table)
                .Where(DB::Query::Where::Equal("PostId", "$PostId"))
                .Where(DB::Query::Where::Equal("TagId", "$TagId"));

        return GetDatabase()
            .Then([](auto db)
                  { return postTagQuery.Prepare(db); })
            .Then([&](auto s)
                  { return s->BindValue("TagId", _TagId)
                        .Then([&](auto s)
                              { return s->BindValue("PostId", _PostId); })
                        .Then([](auto s)
                              { return s->template ExecuteRow<DB::Entities::PostTag>(true); }); });
    }

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // PostFiles
    // ////////////////////////////////////////////////////////////////////////////////////////////

    ExpectedVector<DB::Entities::PostFile> Booru::GetPostFiles()
    {
        return GetDatabase()
            .Then(DB::Entities::GetAll<DB::Entities::PostFile>);
    }

    ExpectedVector<DB::Entities::PostFile> Booru::GetFilesForPost(DB::INTEGER _PostId)
    {
        return GetDatabase()
            .Then([&](auto db)
                  { return DB::Entities::GetAllWithKey<DB::Entities::PostFile>(db, "PostId", _PostId); });
    }

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // PostSiteId
    // ////////////////////////////////////////////////////////////////////////////////////////////

    ExpectedVector<DB::Entities::PostSiteId> Booru::GetPostSiteIds()
    {
        return GetDatabase()
            .Then(DB::Entities::GetAll<DB::Entities::PostSiteId>);
    }

    ExpectedVector<DB::Entities::PostSiteId> Booru::GetPostSiteIdsForPost(DB::INTEGER _PostId)
    {
        return GetDatabase()
            .Then([&](auto db)
                  { return DB::Entities::GetAllWithKey<DB::Entities::PostSiteId>(db, "PostId", _PostId); });
    }

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // Sites
    // ////////////////////////////////////////////////////////////////////////////////////////////

    Expected<DB::Entities::Site> Booru::GetSite(DB::INTEGER _Id)
    {
        return GetDatabase()
            .Then([&](auto db)
                  { return DB::Entities::GetWithKey<DB::Entities::Site>(db, "Id", _Id); });
    }

    Expected<DB::Entities::Site> Booru::GetSite(DB::TEXT const &_Name)
    {
        return GetDatabase()
            .Then([&](auto db)
                  { return DB::Entities::GetWithKey<DB::Entities::Site>(db, "Name", _Name); });
    }

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // Tags
    // ////////////////////////////////////////////////////////////////////////////////////////////

    ExpectedVector<DB::Entities::Tag> Booru::GetTags()
    {
        return GetDatabase()
            .Then(DB::Entities::template GetAll<DB::Entities::Tag>);
    }

    Expected<DB::Entities::Tag> Booru::GetTag(DB::INTEGER _Id)
    {
        return GetDatabase()
            .Then([&](auto db)
                  { return DB::Entities::GetWithKey<DB::Entities::Tag>(db, "Id", _Id); });
    }

    Expected<DB::Entities::Tag> Booru::GetTag(DB::TEXT const &_Name)
    {
        return GetDatabase()
            .Then([&](auto db)
                  { return DB::Entities::GetWithKey<DB::Entities::Tag>(db, "Name", _Name); });
    }

    ExpectedVector<DB::Entities::Tag> Booru::MatchTags(StringView const &_Pattern)
    {
        // make a copy
        String pattern{_Pattern};

        // handle escape sequences and special characters
        String::size_type pos = 0;
        while (pos < pattern.size())
        {
            // escape SQL special characters:
            // "%" -> "\%", "_" -> "\_"
            if (pattern[pos] == '%' || pattern[pos] == '_')
            {
                pattern.insert(pos, 1, '\\');
                pos += 2;
                continue;
            }

            // handle escape
            if (pattern[pos] == '\\')
            {
                // remove escape character and skip over the next
                pattern.erase(pos);
                pos++;
                continue;
            }

            // convert wildcards to SQL
            // "*" -> "%"
            if (pattern[pos] == '*')
            {
                pattern[pos] = '%';
                pos++;
                continue;
            }
            // "?" -> "_"
            if (pattern[pos] == '?')
            {
                pattern[pos] = '_';
                pos++;
                continue;
            }

            //
            pos++;
        }

        // handle wildcards, convert * to %, leave ?
        String::size_type asterPos = pattern.find('*');
        while (asterPos != String::npos)
        {
            pattern[asterPos] = '%';
            asterPos = pattern.find('*');
        }

        return GetDatabase()
            .Then([&](auto db)
                  { return db->PrepareStatement("SELECT * FROM Tags WHERE Name LIKE $Pattern ESCAPE '\\'"); })
            .Then([&](auto s)
                  { return s->BindValue("Pattern", pattern); })
            .Then([&](auto s)
                  { return s->template ExecuteList<DB::Entities::Tag>(); });
    }

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // TagImplications
    // ////////////////////////////////////////////////////////////////////////////////////////////

    ExpectedVector<DB::Entities::TagImplication> Booru::GetTagImplications()
    {
        return GetDatabase()
            .Then(DB::Entities::GetAll<DB::Entities::TagImplication>);
    }

    ExpectedVector<DB::Entities::TagImplication> Booru::GetTagImplicationsForTag(DB::INTEGER _TagId)
    {
        return GetDatabase()
            .Then([&](auto db)
                  { return DB::Entities::GetAllWithKey<DB::Entities::TagImplication>(db, "TagId", _TagId); });
    }

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // TagTypes
    // ////////////////////////////////////////////////////////////////////////////////////////////

    ExpectedVector<DB::Entities::TagType> Booru::GetTagTypes()
    {
        return GetDatabase()
            .Then(DB::Entities::GetAll<DB::Entities::TagType>);
    }

    Expected<DB::Entities::TagType> Booru::GetTagType(DB::INTEGER _Id)
    {
        return GetDatabase()
            .Then([&](auto db)
                  { return DB::Entities::GetWithKey<DB::Entities::TagType>(db, "Id", _Id); });
    }

    Expected<DB::Entities::TagType> Booru::GetTagType(DB::TEXT const &_Name)
    {
        return GetDatabase()
            .Then([&](auto db)
                  { return DB::Entities::GetWithKey<DB::Entities::TagType>(db, "Name", _Name); });
    }

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // Utilities
    // ////////////////////////////////////////////////////////////////////////////////////////////

    Expected<String> Booru::GetSQLConditionForTag(StringView const &_Tag)
    {
        if (_Tag[0] == '-')
        {
            return GetSQLConditionForTag(_Tag.substr(1))
                .Then([](auto c)
                      { return Expected("NOT "s + c); });
        }

        String lowerTag = Strings::ToLower(_Tag);

        // ratings

        if (lowerTag.starts_with("rating:g"))
        {
            // general
            return "( Posts.Rating == 1 ) "s;
        }
        if (lowerTag.starts_with("rating:s"))
        {
            // sensitive
            return "( Posts.Rating == 2 ) "s;
        }
        if (lowerTag.starts_with("rating:q"))
        {
            // questionable + unrated
            return "( Posts.Rating IN ( 0, 3 ) "s;
        }
        if (lowerTag.starts_with("rating:e"))
        {
            // explicit
            return "( Posts.Rating == 4 ) "s;
        }
        if (lowerTag.starts_with("rating:u"))
        {
            // unrated
            return "( Posts.Rating == 0 ) "s;
        }

        // match actual tags

        CHECK_VAR_RETURN_RESULT_ON_ERROR(
            ids,
            MatchTags(_Tag)
                .Then(DB::Entities::CollectIds<DB::Entities::Tag>));

        return "( "
               "   ( "
               "       SELECT  COUNT(*) "
               "       FROM    PostTags "
               "       WHERE   PostTags.PostId =   Posts.Id "
               "       AND     PostTags.TagId  IN  (" +
               Strings::JoinXForm(ids.Value, ", ") +
               ") "
               "   ) > 0 "
               ") ";
    }

    ResultCode Booru::CreateTables()
    {
        CHECK_VAR_RETURN_RESULT_ON_ERROR(db, GetDatabase());
        LOG_INFO("Creating database tables...");

        DB::TransactionGuard guard(db.Value);
        CHECK_RETURN_RESULT_ON_ERROR(db.Value->ExecuteSQL(SQLGetBaseSchema()));
        guard.Commit();

        return ResultCode::CreatedOK;
    }

    ResultCode Booru::UpdateTables(int64_t _Version)
    {
        CHECK_VAR_RETURN_RESULT_ON_ERROR(db, GetDatabase());
        LOG_INFO("Upgrading database schema to version {}...", _Version);

        DB::TransactionGuard guard(db.Value);
        CHECK_RETURN_RESULT_ON_ERROR(db.Value->ExecuteSQL(SQLGetUpdateSchema(_Version)));
        guard.Commit();

        return ResultCode::OK;
    }

} // namespace Booru
