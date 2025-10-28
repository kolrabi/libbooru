#include <booru/booru.hh>

#include <booru/db/entities/post.hh>
#include <booru/db/entities/post_file.hh>
#include <booru/db/entities/post_tag.hh>
#include <booru/db/entities/post_type.hh>
#include <booru/db/entities/site.hh>
#include <booru/db/entities/tag.hh>
#include <booru/db/entities/tag_implication.hh>
#include <booru/db/entities/tag_type.hh>
#include <booru/db/query.hh>
#include <booru/db/stmt.hh>
#include <booru/db/visitors.hh>
#include <booru/log.hh>
#include <booru/result.hh>

#include "db/sql.hh"
#include "db/sqlite3/db.hh"

#include <log4cxx/basicconfigurator.h>

namespace Booru
{

Owning<Booru> Booru::InitializeLibrary() { return Owning<Booru>(new Booru); }

int64_t Booru::GetSchemaVersion() { return SQLGetSchemaVersion(); }

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

ResultCode Booru::OpenDatabase(StringView const& _Path, bool _Create)
{
    LOG_INFO("Opening database at '{}'...", _Path);

    CloseDatabase();

    auto db = DB::Sqlite3::Backend::OpenDatabase(_Path);
    if (!db) { return db.Code; }

    // TODO: other databases

    m_DB = db.Value;

    LOG_INFO("Checking database version...");
    auto dbVersion = GetConfigInt64("db.version");
    if (!dbVersion)
    {
        if (!_Create)
        {
            m_DB = nullptr;
            return ResultCode::InvalidArgument;
        }

        CHECK_RETURN_RESULT_ON_ERROR(CreateTables(), "Table creation failed.");
        dbVersion = GetConfigInt64("db.version");
        CHECK_RETURN_RESULT_ON_ERROR(
            dbVersion.Code,
            "Missing database version even after table creation.");
    }

    LOG_INFO("Sucessfully opened database. Version {}", dbVersion.Value);

    for (DB::INTEGER i = dbVersion.Value; i < SQLGetSchemaVersion(); i++)
    {
        CHECK_RETURN_RESULT_ON_ERROR(UpdateTables(i));
    }

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
    if (m_DB) return {m_DB};
    return {ResultCode::InvalidState};
}

Expected<DB::TEXT> Booru::GetConfig(DB::TEXT const& _Name)
{
    CHECK_VAR_RETURN_RESULT_ON_ERROR(db, GetDatabase());
    CHECK_VAR_RETURN_RESULT_ON_ERROR(
        stmt, DB::Query::Select("Config").Column("Value").Key("Name").Prepare(
                  db.Value));
    CHECK_RETURN_RESULT_ON_ERROR(stmt.Value->BindValue("Name", _Name));
    return stmt.Value->ExecuteScalar<DB::TEXT>();
}

ResultCode Booru::SetConfig(DB::TEXT const& _Name, DB::TEXT const& _Value)
{
    CHECK_VAR_RETURN_RESULT_ON_ERROR(db, GetDatabase());
    CHECK_VAR_RETURN_RESULT_ON_ERROR(
        stmt,
        DB::Query::Upsert("Config").Column("Value").Column("Name").Prepare(
            db.Value));
    CHECK_RETURN_RESULT_ON_ERROR(stmt.Value->BindValue("Name", _Name));
    CHECK_RETURN_RESULT_ON_ERROR(stmt.Value->BindValue("Value", _Value));
    CHECK_RETURN(stmt.Value->StepUpdate());
}

Expected<DB::INTEGER> Booru::GetConfigInt64(DB::TEXT const& _Name)
{
    CHECK_VAR_RETURN_RESULT_ON_ERROR(config, GetConfig(_Name));
    // TODO: error handling
    return std::stoll(config);
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// PostTypes
// ////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Get all post types
ExpectedVector<DB::Entities::PostType> Booru::GetPostTypes()
{
    return GetAll<DB::Entities::PostType>();
}

/// @brief Get post type by Id
Expected<DB::Entities::PostType> Booru::GetPostType(DB::INTEGER _Id)
{
    return Get<DB::Entities::PostType>(_Id);
}

/// @brief Get post type by name
Expected<DB::Entities::PostType> Booru::GetPostType(DB::TEXT const& _Name)
{
    return Get<DB::Entities::PostType>("Name"sv, _Name);
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// Posts
// ////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Get all posts.
ExpectedVector<DB::Entities::Post> Booru::GetPosts()
{
    return GetAll<DB::Entities::Post>();
}

/// @brief Get post by Id.
Expected<DB::Entities::Post> Booru::GetPost(DB::INTEGER _Id)
{
    return Get<DB::Entities::Post>(_Id);
}

/// @brief Get post by hash.
Expected<DB::Entities::Post> Booru::GetPost(DB::BLOB<16> _MD5)
{
    return Get<DB::Entities::Post>("MD5Sum", _MD5);
}

/// @brief Add a tag by name to a post. Considers negation, redirections and
/// implications.
Expected<DB::Entities::Post>
Booru::AddTagToPost(DB::Entities::Post const& _Post,
                    DB::Entities::Tag const& _Tag)
{
    if (_Post.Id == -1 || _Tag.Id == -1) { return ResultCode::InvalidArgument; }

    CHECK_VAR_RETURN_RESULT_ON_ERROR(tag, FollowRedirections(_Tag));
    CHECK_VAR_RETURN_RESULT_ON_ERROR(db, GetDatabase());

    {
        DB::TransactionGuard transactionGuard(db);

        // actually add tag
        DB::Entities::PostTag postTag;
        postTag.PostId    = _Post.Id;
        postTag.TagId     = tag.Value.Id;

        auto createResult = Create(postTag);
        if (createResult.Code == ResultCode::AlreadyExists)
        {
            // already exists
            transactionGuard.Commit();
            return ResultCode::OK;
        }

        // try to honor implications, ignore errors
        CHECK_VAR_RETURN_RESULT_ON_ERROR(implications,
                                         GetTagImplicationsForTag(tag));
        for (auto const& implication : implications.Value)
        {
            DB::INTEGER impliedTagId = implication.ImpliedTagId;
            auto impliedTag          = GetTag(impliedTagId);
            CHECK_CONTINUE_ON_ERROR(impliedTag);

            if (implication.Flags &
                DB::Entities::TagImplication::FLAG_REMOVE_TAG)
            {
                // might fail if tag has not been added, that's fine, ignore
                CHECK(RemoveTagFromPost(_Post, impliedTag));
            }
            else
            {
                // recurse, added tag may have redirection, implications, ...
                // might fail if already added, ignore
                CHECK(AddTagToPost(_Post, impliedTag));
            }
        }

        transactionGuard.Commit();
    }
    return ResultCode::OK;
}

/// @brief Remove a tag by Id from a post .
Expected<DB::Entities::Post>
Booru::RemoveTagFromPost(DB::Entities::Post const& _Post,
                         DB::Entities::Tag const& _Tag)
{
    return {_Post, FindPostTag(_Post.Id, _Tag.Id)
                       .Then([this](auto pt) { return Delete(pt); })
                       .Code};
}

/// @brief Get all posts that match a given query.
ExpectedVector<DB::Entities::Post>
Booru::FindPosts(StringView const& _QueryString)
{
    StringVector queryStringTokens = Strings::Split(_QueryString);
    if (queryStringTokens.empty()) return ResultCode::InvalidRequest;

    auto query = DB::Query::Select(DB::Entities::Post::Table);

    for (auto const& tag : queryStringTokens)
        query.Where(GetSQLConditionForTag(tag));

    return query.Prepare(m_DB).Then(
        &DB::IStmt::ExecuteList<DB::Entities::Post>);
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// PostTags
// ////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Get all post/tag associations.
ExpectedVector<DB::Entities::PostTag> Booru::GetPostTags()
{
    return GetAll<DB::Entities::PostTag>();
}

/// @brief Get all tags for a post by Id.
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
        .Then(&DB::Query::Select::Prepare, tagQuery)
        .Then(DB::IStmt::BindValueFn<DB::INTEGER>(), "PostId", _PostId)
        .Then(&DB::IStmt::ExecuteList<DB::Entities::Tag>);
}

/// @brief Get all posts for a tag by Id.
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
        .Then(&DB::Query::Select::Prepare, postQuery)
        .Then(DB::IStmt::BindValueFn<DB::INTEGER>(), "TagId", _TagId)
        .Then(&DB::IStmt::ExecuteList<DB::Entities::Post>);
}

/// @brief Find a post/tag associations.

Expected<DB::Entities::PostTag> Booru::FindPostTag(DB::INTEGER _PostId,
                                                   DB::INTEGER _TagId)
{
    static auto postTagQuery =
        DB::Query::Select(DB::Entities::PostTag::Table)
            .Where(DB::Query::Where::Equal("PostId", "$PostId"))
            .Where(DB::Query::Where::Equal("TagId", "$TagId"));

    return GetDatabase()
        .Then(&DB::Query::Select::Prepare, postTagQuery)
        .Then(DB::IStmt::BindValueFn<DB::INTEGER>(), "TagId", _TagId)
        .Then(DB::IStmt::BindValueFn<DB::INTEGER>(), "PostId", _PostId)
        .Then(&DB::IStmt::ExecuteRow<DB::Entities::PostTag>, true);
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// PostFiles
// ////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Get all post files.
ExpectedVector<DB::Entities::PostFile> Booru::GetPostFiles()
{
    return GetAll<DB::Entities::PostFile>();
}

/// @brief Get all files for a given post by Id.
ExpectedVector<DB::Entities::PostFile>
Booru::GetFilesForPost(DB::INTEGER _PostId)
{
    return GetAll<DB::Entities::PostFile>("PostId", _PostId);
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// Sites
// ////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Get all sites.
ExpectedVector<DB::Entities::Site> Booru::GetSites()
{
    return GetAll<DB::Entities::Site>();
}

/// @brief Get site by id.
Expected<DB::Entities::Site> Booru::GetSite(DB::INTEGER _Id)
{
    return Get<DB::Entities::Site>("Id", _Id);
}

/// @brief Get all site by name.
Expected<DB::Entities::Site> Booru::GetSite(DB::TEXT const& _Name)
{
    return Get<DB::Entities::Site>("Name", _Name);
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// Tags
// ////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Get all tags.
ExpectedVector<DB::Entities::Tag> Booru::GetTags()
{
    return GetAll<DB::Entities::Tag>();
}

/// @brief Get tag by id.
Expected<DB::Entities::Tag> Booru::GetTag(DB::INTEGER _Id)
{
    return Get<DB::Entities::Tag>("Id", _Id);
}

/// @brief Get all tags by name.
Expected<DB::Entities::Tag> Booru::GetTag(DB::TEXT const& _Name)
{
    return Get<DB::Entities::Tag, DB::TEXT>("Name", _Name);
}

/// @brief Get all tags that match a given pattern.
ExpectedVector<DB::Entities::Tag> Booru::MatchTags(StringView const& _Pattern)
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
        asterPos          = pattern.find('*');
    }

    return GetDatabase()
        .Then(
            [&](auto db)
            {
                return db->PrepareStatement(
                    "SELECT * FROM Tags WHERE Name LIKE $Pattern ESCAPE '\\'");
            })
        .Then(DB::IStmt::BindValueFn<DB::TEXT>(), "Pattern", pattern)
        .Then(&DB::IStmt::ExecuteList<DB::Entities::Tag>);
}

/// @brief Follow the redirection of a tag
Expected<DB::Entities::Tag>
Booru::FollowRedirections(DB::Entities::Tag const& _Tag)
{
    Expected tag  = _Tag;

    // follow redirections
    int iteration = 0;
    while (tag.Value.RedirectId.has_value())
    {
        // TODO: make this a setting
        if (iteration++ > 32) { return ResultCode::RecursionExceeded; }
        CHECK_RETURN_RESULT_ON_ERROR(
            GetTag(tag.Value.RedirectId.value()).Update(tag));
    }
    return tag;
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// TagImplications
// ////////////////////////////////////////////////////////////////////////////////////////////

ExpectedVector<DB::Entities::TagImplication> Booru::GetTagImplications()
{
    return GetAll<DB::Entities::TagImplication>();
}

ExpectedVector<DB::Entities::TagImplication>
Booru::GetTagImplicationsForTag(DB::Entities::Tag const& _Tag)
{
    return GetAll<DB::Entities::TagImplication>("TagId", _Tag.Id);
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// TagTypes
// ////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Get all tag types.
ExpectedVector<DB::Entities::TagType> Booru::GetTagTypes()
{
    return GetAll<DB::Entities::TagType>();
}

/// @brief Get tag type by Id.
Expected<DB::Entities::TagType> Booru::GetTagType(DB::INTEGER _Id)
{
    return Get<DB::Entities::TagType>("Id", _Id);
}

/// @brief Get tag type by name.
Expected<DB::Entities::TagType> Booru::GetTagType(DB::TEXT const& _Name)
{
    return Get<DB::Entities::TagType>("Name", _Name);
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// Utilities
// ////////////////////////////////////////////////////////////////////////////////////////////

Expected<String> Booru::GetSQLConditionForTag(StringView const& _Tag)
{
    if (_Tag[0] == '-')
    {
        return GetSQLConditionForTag(_Tag.substr(1))
            .Then([](auto c) { return Expected("NOT "s + c); });
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
        ids, MatchTags(_Tag).Then(DB::Entities::CollectIds<DB::Entities::Tag>));

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

    StringView sqlSchema     = SQLGetBaseSchema();
    StringVector sqlCommands = Strings::Split(sqlSchema, ';');

    DB::TransactionGuard guard(db.Value);
    for (auto& sql : sqlCommands)
        CHECK_RETURN_RESULT_ON_ERROR(db.Value->ExecuteSQL(sql));
    guard.Commit();

    return ResultCode::CreatedOK;
}

ResultCode Booru::UpdateTables(int64_t _Version)
{
    CHECK_VAR_RETURN_RESULT_ON_ERROR(db, GetDatabase());
    LOG_INFO("Upgrading database schema to version {}...", _Version);

    DB::TransactionGuard guard(db.Value);
    CHECK_RETURN_RESULT_ON_ERROR(
        db.Value->ExecuteSQL(SQLGetUpdateSchema(_Version)));
    guard.Commit();

    return ResultCode::OK;
}

} // namespace Booru
