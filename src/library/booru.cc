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

static inline const String LOGGER = "booru";

Owning<Booru> Booru::InitializeLibrary() 
{ 
    return Owning<Booru>( new Booru ); 
}

Booru::Booru()
{
    log4cxx::BasicConfigurator::configure();
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

    LOG_INFO( "Checking database version..." );
    auto dbVersion = GetConfigInt64( "db.version" );
    if ( !dbVersion )
    {
        if (!_Create)
        {
            m_DB = nullptr;
            return ResultCode::InvalidArgument;
        }
        
        CHECK_RETURN_RESULT_ON_ERROR( CreateTables(), "Table creationg failed." );
        dbVersion = GetConfigInt64( "db.version" );
        CHECK_RETURN_RESULT_ON_ERROR( dbVersion.Code,
                                      "Missing database version even after table creation." );
    }

    for ( DB::INTEGER i = dbVersion.Value; i < k_SQLSchemaVersion; i++ )
    {
        LOG_INFO( "Upgrading database schema to version {}...", i );
        CHECK_RETURN_RESULT_ON_ERROR( UpdateTables( i ) );
    }

    LOG_INFO( "Sucessfully opened database." );
    return ResultCode::OK;
}

void Booru::CloseDatabase()
{
    m_DB = nullptr;
}

DB::DatabaseInterface& Booru::GetDatabase() 
{ 
    return *this->m_DB; 
}

Expected<DB::TEXT> Booru::GetConfig( DB::TEXT const& _Name )
{
    auto stmt = DB::Query::Select( "Config" ).Column( "Value" ).Key( "Name" ).Prepare( *m_DB );
    CHECK_RETURN_RESULT_ON_ERROR( stmt );
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Value->BindValue( "Name", _Name ) );

    Expected<DB::TEXT> value = stmt.Value->ExecuteScalar<DB::TEXT>();
    LOG_INFO( "Config {} = '{}'", _Name, value.Value );
    CHECK_RETURN_RESULT_ON_ERROR( value );
    return value;
}

Expected<DB::INTEGER> Booru::GetConfigInt64( DB::TEXT const& _Key )
{
    auto config = GetConfig( _Key );
    if ( config )
    {
        // TODO: error handling
        return ::atol( config.Value.c_str() );
    }
    return config.Code;
}

ResultCode Booru::CreateTables()
{
    LOG_INFO( "Creating database tables..." );
    return this->m_DB->ExecuteSQL( g_SQLSchema );
}

ResultCode Booru::UpdateTables( int64_t _Version )
{
    return this->m_DB->ExecuteSQL( SQLGetUpdateSchema( _Version ) );
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// PostTypes
// ////////////////////////////////////////////////////////////////////////////////////////////

ResultCode Booru::CreatePostType( DB::Entities::PostType& _PostType )
{
    return DB::Entities::Create( *m_DB, _PostType );
}

ExpectedList<DB::Entities::PostType> Booru::GetPostTypes()
{
    return DB::Entities::GetAll<DB::Entities::PostType>( *m_DB );
}

Expected<DB::Entities::PostType> Booru::GetPostType( DB::INTEGER _Id )
{
    return DB::Entities::GetWithKey<DB::Entities::PostType>( *m_DB, "Id", _Id );
}

Expected<DB::Entities::PostType> Booru::GetPostType( DB::TEXT const & _Name )
{
    return DB::Entities::GetWithKey<DB::Entities::PostType>( *m_DB, "Name", _Name );
}

ResultCode Booru::UpdatePostType( DB::Entities::PostType& _PostType )
{
    return DB::Entities::Update( *m_DB, _PostType );
}

ResultCode Booru::DeletePostType( DB::Entities::PostType& _PostType )
{
    return DB::Entities::Delete( *m_DB, _PostType );
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// Posts
// ////////////////////////////////////////////////////////////////////////////////////////////

ResultCode Booru::CreatePost( DB::Entities::Post& _Post ) 
{ 
    return DB::Entities::Create( *m_DB, _Post ); 
}

ExpectedList<DB::Entities::Post> Booru::GetPosts()
{
    auto result = DB::Entities::GetAll<DB::Entities::Post>( *m_DB );
    CHECK_RETURN_RESULT_ON_ERROR( result, "Failed to get posts" );
    LOG_INFO( "Retrieved all {} posts", result.Value.size() );
    return result;
}

Expected<DB::Entities::Post> Booru::GetPost( DB::INTEGER _Id )
{
    auto result = DB::Entities::GetWithKey<DB::Entities::Post>( *m_DB, "Id", _Id );
    CHECK_RETURN_RESULT_ON_ERROR( result, "Could not get post by id {}", _Id );
    LOG_INFO( "Retrieved post by id: {}", _Id );
    return result;
}

Expected<DB::Entities::Post> Booru::GetPost( DB::BLOB<16> _MD5 )
{
    auto result = DB::Entities::GetWithKey<DB::Entities::Post>( *m_DB, "MD5Sum", _MD5 );
    CHECK_RETURN_RESULT_ON_ERROR( result, "Could not get post by md5 {}", Strings::From( _MD5 ) );
    LOG_INFO( "Retrieved post by md5 {}: {}", Strings::From( _MD5 ), result.Value.Id );
    return result;
}

ResultCode Booru::UpdatePost( DB::Entities::Post& _Post )
{
    auto result = DB::Entities::Update( *m_DB, _Post );
    CHECK_RETURN_RESULT_ON_ERROR( result, "Could not update post {}", _Post.Id );
    LOG_INFO( "Successfully updated post {}", _Post.Id );
    return result;
}

ResultCode Booru::DeletePost( DB::Entities::Post& _Post )
{
    auto result = DB::Entities::Delete( *m_DB, _Post );
    CHECK_RETURN_RESULT_ON_ERROR( result, "Could not delete post {}", _Post.Id );
    LOG_INFO( "Successfully deleted post {}", _Post.Id );
    return result;
}

ResultCode Booru::AddTagToPost( DB::INTEGER _PostId, StringView const & _Tag )
{
    StringView tagName = _Tag;
    bool remove = _Tag[0] == '-';
    if ( remove )
    {
        tagName = _Tag.substr(1);
    }

    Expected<DB::Entities::Tag> tag = GetTag( DB::TEXT(tagName) );
    CHECK_RETURN_RESULT_ON_ERROR( tag.Code );

    DB::Entities::PostTag postTag;
    postTag.PostId = _PostId;
    postTag.TagId  = tag.Value.Id;

    if ( remove )
    {
        return DeletePostTag( postTag );
    }

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

    DB::TransactionGuard transactionGuard( *m_DB );

    // actually add tag
    auto createPostTagResult = CreatePostTag( postTag );
    if ( createPostTagResult != ResultCode::AlreadyExists )
    {
        CHECK_RETURN_RESULT_ON_ERROR( createPostTagResult );
    }

    // try to honor implications, ignore errors
    ExpectedList<DB::Entities::TagImplication> implications =
        GetTagImplicationsForTag( tag.Value.Id );
    CHECK_RETURN_RESULT_ON_ERROR( implications );
    for ( auto const& implication : implications.Value )
    {
        DB::INTEGER impliedTagId = implication.ImpliedTagId;
        if ( implication.Flags & DB::Entities::TagImplication::FLAG_REMOVE_TAG )
        {
            DB::Entities::PostTag postTag;
            postTag.PostId = _PostId;
            postTag.TagId  = tag.Value.Id;
            CHECK_RETURN_RESULT_ON_ERROR( DeletePostTag( postTag ) );
        }
        else
        {
            Expected<DB::Entities::Tag> impliedTag = GetTag( impliedTagId );
            CHECK_RETURN_RESULT_ON_ERROR( impliedTag );

            fprintf( stderr, "%ld -> %ld\n", tag.Value.Id, impliedTagId );

            // recurse, added tag may have redirection, implications, ...
            CHECK_RETURN_RESULT_ON_ERROR( AddTagToPost( _PostId, impliedTag.Value.Name.c_str() ) );
        }
    }

    transactionGuard.Commit();
    return ResultCode::OK;
}

ExpectedList<DB::Entities::Post> Booru::FindPosts( StringView const & _QueryString )
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

    std::vector<String> conditionStrings;
    for ( auto const& tag : queryStringTokens )
    {
        conditionStrings.push_back( GetSQLConditionForTag( tag ) );
    }

    SQL += Strings::Join( conditionStrings, " AND \n" );

    auto stmt = m_DB->PrepareStatement( SQL.c_str() );
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Code );
    return stmt.Value->ExecuteList<DB::Entities::Post>();
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// PostTags
// ////////////////////////////////////////////////////////////////////////////////////////////

ResultCode Booru::CreatePostTag( DB::Entities::PostTag& _PostTag )
{
    auto stmt = DB::Query::Insert( "PostTags" ).Column( "PostId" ).Column( "TagId" ).Prepare( *m_DB );
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Code );
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Value->BindValue( "PostId", _PostTag.PostId ) );
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Value->BindValue( "TagId", _PostTag.TagId ) );
    return stmt.Value->Step();
}

ExpectedList<DB::Entities::PostTag> Booru::GetPostTags() 
{ 
    return DB::Entities::GetAll<DB::Entities::PostTag>( *m_DB ); 
}

ResultCode Booru::DeletePostTag( DB::Entities::PostTag& _PostTag )
{
    auto stmt = DB::Query::Delete( "PostTags" ).Key( "PostId" ).Key( "TagId" ).Prepare( *m_DB );
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Code );
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Value->BindValue( "PostId", _PostTag.PostId ) );
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Value->BindValue( "TagId", _PostTag.TagId ) );
    return stmt.Value->Step();
}

ExpectedList<DB::Entities::Tag> Booru::GetTagsForPost( DB::INTEGER _PostId )
{
    auto stmt = m_DB->PrepareStatement( R"SQL(
        SELECT * FROM Tags WHERE Tags.Id IN
            ( SELECT TagId FROM PostTags WHERE PostTags.PostId = $PostId ) 
        )SQL" );
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Code );
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Value->BindValue( "PostId", _PostId ) );
    return stmt.Value->ExecuteList<DB::Entities::Tag>();
}

ExpectedList<DB::Entities::Post> Booru::GetPostsForTag( DB::INTEGER _TagId )
{
    auto stmt = m_DB->PrepareStatement( R"SQL(
        SELECT * FROM Posts WHERE Posts.Id IN
            ( SELECT PostId FROM PostTags WHERE PostTags.TagId = $TagId ) 
        )SQL" );
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Code );
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Value->BindValue( "TagId", _TagId ) );
    return stmt.Value->ExecuteList<DB::Entities::Post>();
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// PostFiles
// ////////////////////////////////////////////////////////////////////////////////////////////

ResultCode Booru::CreatePostFile( DB::Entities::PostFile& _PostFile )
{
    return DB::Entities::Create( *m_DB, _PostFile );
}

ExpectedList<DB::Entities::PostFile> Booru::GetPostFiles()
{
    return DB::Entities::GetAll<DB::Entities::PostFile>( *m_DB );
}

ExpectedList<DB::Entities::PostFile> Booru::GetFilesForPost( DB::INTEGER _PostId )
{
    return DB::Entities::GetAllWithKey<DB::Entities::PostFile>( *m_DB, "PostId", _PostId );
}

ResultCode Booru::DeletePostFile( DB::Entities::PostFile& _PostFile )
{
    return DB::Entities::Delete( *m_DB, _PostFile );
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// PostSiteId
// ////////////////////////////////////////////////////////////////////////////////////////////

ResultCode Booru::CreatePostSiteId( DB::Entities::PostSiteId& _PostSiteId )
{
    return DB::Entities::Create( *m_DB, _PostSiteId );
}

ExpectedList<DB::Entities::PostSiteId> Booru::GetPostSiteIds()
{
    return DB::Entities::GetAll<DB::Entities::PostSiteId>( *m_DB );
}

ExpectedList<DB::Entities::PostSiteId> Booru::GetPostSiteIdsForPost( DB::INTEGER _PostId )
{
    return DB::Entities::GetAllWithKey<DB::Entities::PostSiteId>( *m_DB, "PostId", _PostId );
}

ResultCode Booru::DeletePostSiteId( DB::Entities::PostSiteId& _PostSiteId )
{
    return DB::Entities::Delete( *m_DB, _PostSiteId );
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// Sites
// ////////////////////////////////////////////////////////////////////////////////////////////

ResultCode Booru::CreateSite( DB::Entities::Site& _Site ) { return DB::Entities::Create( *m_DB, _Site ); }

Expected<DB::Entities::Site> Booru::GetSite( DB::INTEGER _Id )
{
    return DB::Entities::GetWithKey<DB::Entities::Site>( *m_DB, "Id", _Id );
}

Expected<DB::Entities::Site> Booru::GetSite( DB::TEXT const & _Name )
{
    return DB::Entities::GetWithKey<DB::Entities::Site>( *m_DB, "Name", _Name );
}

ResultCode Booru::UpdateSite( DB::Entities::Site& _Site ) { return DB::Entities::Update( *m_DB, _Site ); }

ResultCode Booru::DeleteSite( DB::Entities::Site& _Site ) { return DB::Entities::Delete( *m_DB, _Site ); }

// ////////////////////////////////////////////////////////////////////////////////////////////
// Tags
// ////////////////////////////////////////////////////////////////////////////////////////////

ResultCode Booru::CreateTag( DB::Entities::Tag& _Tag ) { return DB::Entities::Create( *m_DB, _Tag ); }

ExpectedList<DB::Entities::Tag> Booru::GetTags() { return DB::Entities::GetAll<DB::Entities::Tag>( *m_DB ); }

Expected<DB::Entities::Tag> Booru::GetTag( DB::INTEGER _Id )
{
    return DB::Entities::GetWithKey<DB::Entities::Tag>( *m_DB, "Id", _Id );
}

Expected<DB::Entities::Tag> Booru::GetTag( DB::TEXT const & _Name )
{
    return DB::Entities::GetWithKey<DB::Entities::Tag>( *m_DB, "Name", _Name );
}

ResultCode Booru::UpdateTag( DB::Entities::Tag& _Tag ) { return DB::Entities::Update( *m_DB, _Tag ); }

ResultCode Booru::DeleteTag( DB::Entities::Tag& _Tag ) { return DB::Entities::Delete( *m_DB, _Tag ); }

ExpectedList<DB::Entities::Tag> Booru::MatchTags( DB::TEXT& _Pattern )
{
    String pattern = _Pattern;

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

    String::size_type asterPos = pattern.find( '*' );
    while ( asterPos != String::npos )
    {
        pattern[asterPos] = '%';
        asterPos          = pattern.find( '*' );
    }

    auto stmt = m_DB->PrepareStatement( "SELECT * FROM Tags WHERE Name LIKE $Pattern ESCAPE '\\'" );
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Code );
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Value->BindValue( "Pattern", pattern ) );
    return stmt.Value->ExecuteList<DB::Entities::Tag>();
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// TagImplications
// ////////////////////////////////////////////////////////////////////////////////////////////

ResultCode Booru::CreateTagImplication( DB::Entities::TagImplication& _TagImplication )
{
    if ( _TagImplication.TagId == _TagImplication.ImpliedTagId )
    {
        return ResultCode::InvalidArgument;
    }
    return DB::Entities::Create( *m_DB, _TagImplication );
}

ExpectedList<DB::Entities::TagImplication> Booru::GetTagImplications()
{
    return DB::Entities::GetAll<DB::Entities::TagImplication>( *m_DB );
}

ExpectedList<DB::Entities::TagImplication> Booru::GetTagImplicationsForTag( DB::INTEGER _TagId )
{
    return DB::Entities::GetAllWithKey<DB::Entities::TagImplication>( *m_DB, "TagId", _TagId );
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// TagTypes
// ////////////////////////////////////////////////////////////////////////////////////////////

ResultCode Booru::CreateTagType( DB::Entities::TagType& _TagType ) { return DB::Entities::Create( *m_DB, _TagType ); }

ExpectedList<DB::Entities::TagType> Booru::GetTagTypes() { return DB::Entities::GetAll<DB::Entities::TagType>( *m_DB ); }

Expected<DB::Entities::TagType> Booru::GetTagType( DB::INTEGER _Id )
{
    return DB::Entities::GetWithKey<DB::Entities::TagType>( *m_DB, "Id", _Id );
}

Expected<DB::Entities::TagType> Booru::GetTagType( DB::TEXT const & _Name )
{
    return DB::Entities::GetWithKey<DB::Entities::TagType>( *m_DB, "Name", _Name );
}

ResultCode Booru::UpdateTagType( DB::Entities::TagType& _TagType ) { return DB::Entities::Update( *m_DB, _TagType ); }

ResultCode Booru::DeleteTagType( DB::Entities::TagType& _TagType ) { return DB::Entities::Delete( *m_DB, _TagType ); }

// ////////////////////////////////////////////////////////////////////////////////////////////
// Utilities
// ////////////////////////////////////////////////////////////////////////////////////////////

Expected<String> Booru::GetSQLConditionForTag( String _Tag )
{
    using namespace std::literals;

    if ( _Tag[0] == '-' )
    {
        Expected<String> subCondition = GetSQLConditionForTag( _Tag.substr( 1 ) );
        CHECK_RETURN_RESULT_ON_ERROR( subCondition );
        return "NOT " + subCondition.Value;
    }

    String lowerTag = _Tag;
    std::transform( begin( lowerTag ), end( lowerTag ), begin( lowerTag ),
                    []( char c ) { return std::tolower( c ); } );

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

    ExpectedList<DB::Entities::Tag> tags = MatchTags( _Tag );
    CHECK_RETURN_RESULT_ON_ERROR( tags );

    return "( "
           "   ( "
           "       SELECT  COUNT(*) "
           "       FROM    PostTags "
           "       WHERE   PostTags.PostId =   Posts.Id "
           "       AND     PostTags.TagId  IN  (" +
           Strings::Join( CollectIds( tags.Value ) ) +
           ") "
           "   ) > 0 "
           ") ";
}

} // namespace Booru
