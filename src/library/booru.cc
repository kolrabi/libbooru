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
    return Owning<Booru>( new Booru ); 
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

    LOG_INFO( "Booru library initialized" );
}

Booru::~Booru() 
{ 
    LOG_INFO( "Booru library shutting down" ); 
    CloseDatabase();
}

ResultCode Booru::OpenDatabase( StringView const & _Path, bool _Create )
{
    LOG_INFO( "Opening database at '{}'...", _Path );

    CloseDatabase();

    auto db = DB::Sqlite3::DatabaseSqlite3::OpenDatabase( _Path );
    if ( !db )
    {
        return db.Code;
    }

    // TODO: other databases

    m_DB = std::move( db.Value );

    DB::TransactionGuard guard(*m_DB);

    LOG_INFO( "Checking database version..." );
    auto dbVersion = GetConfigInt64( "db.version" );
    if ( !dbVersion )
    {
        if (!_Create)
        {
            guard.Rollback();
            m_DB = nullptr;
            return ResultCode::InvalidArgument;
        }
        
        CHECK_RETURN_RESULT_ON_ERROR( CreateTables(), "Table creationg failed." );
        dbVersion = GetConfigInt64( "db.version" );
        CHECK_RETURN_RESULT_ON_ERROR( dbVersion.Code,
                                      "Missing database version even after table creation." );
    }

    LOG_INFO( "Sucessfully opened database. Version {}", dbVersion.Value );

    for ( DB::INTEGER i = dbVersion.Value; i < SQLGetSchemaVersion(); i++ )
    {
        CHECK_RETURN_RESULT_ON_ERROR( UpdateTables( i ) );
    }

    guard.Commit();

    return ResultCode::OK;
}

void Booru::CloseDatabase()
{
    if ( m_DB ) 
    {
        CHECK( m_DB->RollbackTransaction() );
    }
    m_DB = nullptr;
}

Expected<DB::DatabaseInterface*> Booru::GetDatabase() 
{ 
    if (m_DB)
        return this->m_DB.get(); 
    return ResultCode::InvalidState;
}

Expected<DB::TEXT> Booru::GetConfig( DB::TEXT const& _Name )
{
    CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
    CHECK_VAR_RETURN_RESULT_ON_ERROR( stmt, DB::Query::Select( "Config" ).Column( "Value" ).Key( "Name" ).Prepare( *db ) );
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Value->BindValue( "Name", _Name ) );
    return stmt.Value->ExecuteScalar<DB::TEXT>();
}

ResultCode Booru::SetConfig( DB::TEXT const & _Name, DB::TEXT const & _Value )
{
    CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
    CHECK_VAR_RETURN_RESULT_ON_ERROR( stmt, DB::Query::Upsert( "Config" ).Column( "Value" ).Column( "Name" ).Prepare( *db ) );
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Value->BindValue( "Name", _Name ) );
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Value->BindValue( "Value", _Value ) );
    CHECK_RETURN( stmt.Value->Step(false) );
}

Expected<DB::INTEGER> Booru::GetConfigInt64( DB::TEXT const& _Name )
{
    CHECK_VAR_RETURN_RESULT_ON_ERROR( config, GetConfig( _Name ) );
    // TODO: error handling
    return std::stoll( config );
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// PostTypes
// ////////////////////////////////////////////////////////////////////////////////////////////

ExpectedVector<DB::Entities::PostType> Booru::GetPostTypes()
{
    CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
    return DB::Entities::GetAll<DB::Entities::PostType>( *db );
}

Expected<DB::Entities::PostType> Booru::GetPostType( DB::INTEGER _Id )
{
    CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
    return DB::Entities::GetWithKey<DB::Entities::PostType>( *db, "Id", _Id );
}

Expected<DB::Entities::PostType> Booru::GetPostType( DB::TEXT const & _Name )
{
    CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
    return DB::Entities::GetWithKey<DB::Entities::PostType>( *db, "Name", _Name );
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// Posts
// ////////////////////////////////////////////////////////////////////////////////////////////

ExpectedVector<DB::Entities::Post> Booru::GetPosts()
{
    CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
    return DB::Entities::GetAll<DB::Entities::Post>( *db );
}

Expected<DB::Entities::Post> Booru::GetPost( DB::INTEGER _Id )
{
    CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
    return DB::Entities::GetWithKey<DB::Entities::Post>( *db, "Id", _Id );
}

Expected<DB::Entities::Post> Booru::GetPost( DB::BLOB<16> _MD5 )
{
    CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
    return DB::Entities::GetWithKey<DB::Entities::Post>( *db, "MD5Sum", _MD5 );
}

ResultCode Booru::AddTagToPost( DB::INTEGER _PostId, StringView const & _Tag )
{
    StringView tagName = _Tag;
    bool remove = _Tag[0] == '-';
    if ( remove )
    {
        tagName = tagName.substr(1);
    }

    // find tag
    CHECK_VAR_RETURN_RESULT_ON_ERROR( tag, GetTag( DB::TEXT(tagName) ) );

    if ( remove )
        return RemoveTagFromPost( _PostId, tag.Value.Id );

    // follow redirections
    int iteration = 0;
    while ( tag.Value.RedirectId.has_value() )
    {
        // TODO: make this a setting
        if ( iteration++ > 32 )
        {
            return ResultCode::RecursionExceeded;
        }
        tag = GetTag( tag.Value.RedirectId.value() );
        CHECK_RETURN_RESULT_ON_ERROR( tag );
    }

    CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
    DB::TransactionGuard transactionGuard( *db );

    // actually add tag
    DB::Entities::PostTag postTag;
    postTag.PostId = _PostId;
    postTag.TagId  = tag.Value.Id;

    CHECK_RETURN_RESULT_ON_ERROR( Create( postTag ) );

    // try to honor implications, ignore errors
    CHECK_VAR_RETURN_RESULT_ON_ERROR( implications, GetTagImplicationsForTag( tag.Value.Id ) );
    for ( auto const& implication : implications.Value )
    {
        DB::INTEGER impliedTagId = implication.ImpliedTagId;
        if ( implication.Flags & DB::Entities::TagImplication::FLAG_REMOVE_TAG )
        {
            // might fail if tag has not been added, that's fine, ignore
            CHECK( RemoveTagFromPost( _PostId, tag.Value.Id ) );
        }
        else
        {
            auto impliedTag = GetTag( impliedTagId );
            CHECK_CONTINUE_ON_ERROR( impliedTag );

            // recurse, added tag may have redirection, implications, ...
            // might fail if already added, ignore
            CHECK( AddTagToPost( _PostId, impliedTag.Value.Name.c_str() ) );
        }
    }

    transactionGuard.Commit();
    return ResultCode::OK;
}

ResultCode Booru::RemoveTagFromPost( DB::INTEGER _PostId, DB::INTEGER _TagId )
{
    CHECK_VAR_RETURN_RESULT_ON_ERROR( postTag, FindPostTag( _PostId, _TagId ) );
    return Delete( postTag.Value );
}

ExpectedVector<DB::Entities::Post> Booru::FindPosts( StringView const & _QueryString )
{
    StringVector queryStringTokens = Strings::Split( _QueryString );
    if ( queryStringTokens.empty() )
    {
        return ResultCode::InvalidRequest;
    }

    String SQL = R"SQL(
        SELECT  *
        FROM    Posts
        WHERE
    )SQL";

    StringVector conditionStrings;
    for ( auto const& tag : queryStringTokens )
    {
        conditionStrings.push_back( GetSQLConditionForTag( tag ) );
    }

    SQL += Strings::Join( conditionStrings, " AND \n" );

    auto stmt = m_DB->PrepareStatement( SQL );
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Code );
    return stmt.Value->ExecuteList<DB::Entities::Post>();
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// PostTags
// ////////////////////////////////////////////////////////////////////////////////////////////

ExpectedVector<DB::Entities::PostTag> Booru::GetPostTags() 
{ 
    CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
    return DB::Entities::GetAll<DB::Entities::PostTag>( *db ); 
}

ExpectedVector<DB::Entities::Tag> Booru::GetTagsForPost( DB::INTEGER _PostId )
{
    CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
    CHECK_VAR_RETURN_RESULT_ON_ERROR( stmt, db.Value->PrepareStatement( R"SQL(
        SELECT * FROM Tags WHERE Tags.Id IN
            ( SELECT TagId FROM PostTags WHERE PostTags.PostId = $PostId ) 
        )SQL" ));
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Value->BindValue( "PostId", _PostId ) );
    return stmt.Value->ExecuteList<DB::Entities::Tag>();
}

ExpectedVector<DB::Entities::Post> Booru::GetPostsForTag( DB::INTEGER _TagId )
{
    CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
    CHECK_VAR_RETURN_RESULT_ON_ERROR( stmt, db.Value->PrepareStatement( R"SQL(
        SELECT * FROM Posts WHERE Posts.Id IN
            ( SELECT PostId FROM PostTags WHERE PostTags.TagId = $TagId ) 
        )SQL" ));
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Value->BindValue( "TagId", _TagId ) );
    return stmt.Value->ExecuteList<DB::Entities::Post>();
}

Expected<DB::Entities::PostTag> Booru::FindPostTag( DB::INTEGER _PostId, DB::INTEGER _TagId )
{
    CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
    CHECK_VAR_RETURN_RESULT_ON_ERROR( stmt, db.Value->PrepareStatement( "SELECT * FROM PostTags WHERE PostId = $PostId AND TagId = $TagId" ) );
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Value->BindValue( "PostId", _PostId ) );
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Value->BindValue( "TagId", _TagId ) );
    return stmt.Value->ExecuteRow<DB::Entities::PostTag>();
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// PostFiles
// ////////////////////////////////////////////////////////////////////////////////////////////

ExpectedVector<DB::Entities::PostFile> Booru::GetPostFiles()
{
    CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
    return DB::Entities::GetAll<DB::Entities::PostFile>( *db );
}

ExpectedVector<DB::Entities::PostFile> Booru::GetFilesForPost( DB::INTEGER _PostId )
{
    CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
    return DB::Entities::GetAllWithKey<DB::Entities::PostFile>( *db, "PostId", _PostId );
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// PostSiteId
// ////////////////////////////////////////////////////////////////////////////////////////////

ExpectedVector<DB::Entities::PostSiteId> Booru::GetPostSiteIds()
{
    CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
    return DB::Entities::GetAll<DB::Entities::PostSiteId>( *db );
}

ExpectedVector<DB::Entities::PostSiteId> Booru::GetPostSiteIdsForPost( DB::INTEGER _PostId )
{
    CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
    return DB::Entities::GetAllWithKey<DB::Entities::PostSiteId>( *db, "PostId", _PostId );
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// Sites
// ////////////////////////////////////////////////////////////////////////////////////////////

Expected<DB::Entities::Site> Booru::GetSite( DB::INTEGER _Id )
{
    CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
    return DB::Entities::GetWithKey<DB::Entities::Site>( *db, "Id", _Id );
}

Expected<DB::Entities::Site> Booru::GetSite( DB::TEXT const & _Name )
{
    CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
    return DB::Entities::GetWithKey<DB::Entities::Site>( *db, "Name", _Name );
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// Tags
// ////////////////////////////////////////////////////////////////////////////////////////////


ExpectedVector<DB::Entities::Tag> Booru::GetTags()
{ 
    CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
    return DB::Entities::GetAll<DB::Entities::Tag>( *db ); 
}

Expected<DB::Entities::Tag> Booru::GetTag( DB::INTEGER _Id )
{
    CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
    return DB::Entities::GetWithKey<DB::Entities::Tag>( *db, "Id", _Id );
}

Expected<DB::Entities::Tag> Booru::GetTag( DB::TEXT const & _Name )
{
    CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
    return DB::Entities::GetWithKey<DB::Entities::Tag>( *db, "Name", _Name );
}

ExpectedVector<DB::Entities::Tag> Booru::MatchTags( StringView const & _Pattern )
{
    // make a copy
    String pattern { _Pattern };

    // handle escape sequences and special characters
    String::size_type pos = 0;
    while ( pos < pattern.size() )
    {
        // escape SQL special characters:
        // "%" -> "\%", "_" -> "\_"
        if ( pattern[pos] == '%' || pattern[pos] == '_' )
        {
            pattern.insert( pos, 1, '\\' );
            pos += 2;
            continue;
        }

        // handle escape
        if ( pattern[pos] == '\\' )
        {
            // remove escape character and skip over the next
            pattern.erase( pos );
            pos++;
            continue;
        }

        // convert wildcards to SQL
        // "*" -> "%"
        if ( pattern[pos] == '*' )
        {
            pattern[pos] = '%';
            pos++;
            continue;
        }
        // "?" -> "_"
        if ( pattern[pos] == '?' )
        {
            pattern[pos] = '_';
            pos++;
            continue;
        }

        //
        pos++;
    }

    // handle wildcards, convert * to %, leave ?
    String::size_type asterPos = pattern.find( '*' );
    while ( asterPos != String::npos )
    {
        pattern[asterPos] = '%';
        asterPos          = pattern.find( '*' );
    }

    CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
    CHECK_VAR_RETURN_RESULT_ON_ERROR(stmt, db.Value->PrepareStatement( "SELECT * FROM Tags WHERE Name LIKE $Pattern ESCAPE '\\'" ) );
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Value->BindValue( "Pattern", pattern ) );
    return stmt.Value->ExecuteList<DB::Entities::Tag>();
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// TagImplications
// ////////////////////////////////////////////////////////////////////////////////////////////

ExpectedVector<DB::Entities::TagImplication> Booru::GetTagImplications()
{
    CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
    return DB::Entities::GetAll<DB::Entities::TagImplication>( *db );
}

ExpectedVector<DB::Entities::TagImplication> Booru::GetTagImplicationsForTag( DB::INTEGER _TagId )
{
    CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
    return DB::Entities::GetAllWithKey<DB::Entities::TagImplication>( *db, "TagId", _TagId );
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// TagTypes
// ////////////////////////////////////////////////////////////////////////////////////////////

ExpectedVector<DB::Entities::TagType> Booru::GetTagTypes() 
{ 
    CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
    return DB::Entities::GetAll<DB::Entities::TagType>( *db ); 
}

Expected<DB::Entities::TagType> Booru::GetTagType( DB::INTEGER _Id )
{
    CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
    return DB::Entities::GetWithKey<DB::Entities::TagType>( *db, "Id", _Id );
}

Expected<DB::Entities::TagType> Booru::GetTagType( DB::TEXT const & _Name )
{
    CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
    return DB::Entities::GetWithKey<DB::Entities::TagType>( *db, "Name", _Name );
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// Utilities
// ////////////////////////////////////////////////////////////////////////////////////////////

Expected<String> Booru::GetSQLConditionForTag( StringView const & _Tag )
{
    if ( _Tag[0] == '-' )
    {
        Expected<String> subCondition = GetSQLConditionForTag( _Tag.substr( 1 ) );
        CHECK_RETURN_RESULT_ON_ERROR( subCondition );
        return "NOT " + subCondition.Value;
    }

    String lowerTag = Strings::ToLower( _Tag );

    // ratings

    if ( lowerTag.starts_with( "rating:g" ) )
    {
        // general
        return "( Posts.Rating == 1 ) "s;
    }
    if ( lowerTag.starts_with( "rating:s" ) )
    {
        // sensitive
        return "( Posts.Rating == 2 ) "s;
    }
    if ( lowerTag.starts_with( "rating:q" ) )
    {
        // questionable + unrated
        return "( Posts.Rating IN ( 0, 3 ) "s;
    }
    if ( lowerTag.starts_with( "rating:e" ) )
    {
        // explicit
        return "( Posts.Rating == 4 ) "s;
    }
    if ( lowerTag.starts_with( "rating:u" ) )
    {
        // unrated
        return "( Posts.Rating == 0 ) "s;
    }

    // match actual tags

    CHECK_VAR_RETURN_RESULT_ON_ERROR( tags, MatchTags( _Tag ) );

    return "( "
           "   ( "
           "       SELECT  COUNT(*) "
           "       FROM    PostTags "
           "       WHERE   PostTags.PostId =   Posts.Id "
           "       AND     PostTags.TagId  IN  (" +
           Strings::JoinXForm( CollectIds( tags.Value ), ", " ) +
           ") "
           "   ) > 0 "
           ") ";
}

ResultCode Booru::CreateTables()
{
    CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
    LOG_INFO( "Creating database tables..." );

    DB::TransactionGuard guard(*db);
    CHECK_RETURN_RESULT_ON_ERROR( db.Value->ExecuteSQL( SQLGetBaseSchema() ) );
    guard.Commit();

    return ResultCode::CreatedOK;
}

ResultCode Booru::UpdateTables( int64_t _Version )
{
    CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
    LOG_INFO( "Upgrading database schema to version {}...", _Version );

    DB::TransactionGuard guard(*db);
    CHECK_RETURN_RESULT_ON_ERROR( db.Value->ExecuteSQL( SQLGetUpdateSchema( _Version ) ) );
    guard.Commit();

    return ResultCode::OK;
}


} // namespace Booru
