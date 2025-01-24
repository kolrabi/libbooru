#include "booru/booru.hh"

#include "booru/db/entities/post.hh"
#include "booru/db/entities/post_file.hh"
#include "booru/db/entities/post_site_id.hh"
#include "booru/db/entities/post_tag.hh"
#include "booru/db/entities/post_type.hh"
#include "booru/db/entities/site.hh"
#include "booru/db/entities/tag.hh"
#include "booru/db/entities/tag_implication.hh"
#include "booru/db/entities/tag_type.hh"
#include "booru/db/query.hh"
#include "booru/db/visitors.hh"
#include "booru/result.hh"
#include "db/sql.hh"
#include "db/sqlite3/db.hh"

#include <iostream>
#include <mutex>
#include <algorithm>

namespace Booru
{

using namespace DB::Entities;

static std::mutex g_BooruMutex;

std::unique_ptr<Booru> Booru::InitializeLibrary() { return std::make_unique<Booru>(); }

template <class TEntity>
static ResultCode CreateEntity( DB::DatabaseInterface& _DB, TEntity& _Entity )
{
    auto stmt = DB::InsertEntity<TEntity>( TEntity::Table, _Entity ).Prepare( _DB );
    CHECK_RESULT_RETURN_ERROR( stmt.Code );
    CHECK_RESULT_RETURN_ERROR( StoreEntity( _Entity, *stmt.Value ) );
    CHECK_RESULT_RETURN_ERROR( stmt.Value->Step() );
    return _DB.GetLastRowId( _Entity.Id );
}

template <class TEntity, class TValue>
static Expected<TEntity> GetEntity( DB::DatabaseInterface& _DB, char const* _KeyColumn,
                                    TValue const& _KeyValue )
{
    auto stmt = DB::Select( TEntity::Table ).Key( _KeyColumn ).Prepare( _DB );
    CHECK_RESULT_RETURN_ERROR_PRINT( stmt.Code );
    CHECK_RESULT_RETURN_ERROR_PRINT(
        stmt.Value->BindValue( _KeyColumn, _KeyValue ) ); // $ + KeyColumn?
    return stmt.Value->ExecuteRow<TEntity>( true );
}

template <class TEntity>
static ExpectedList<TEntity> GetEntities( DB::DatabaseInterface& _DB )
{
    auto stmt = DB::Select( TEntity::Table ).Prepare( _DB );
    CHECK_RESULT_RETURN_ERROR( stmt.Code );
    return stmt.Value->ExecuteList<TEntity>();
}

template <class TEntity, class TValue>
static ExpectedList<TEntity> GetEntities( DB::DatabaseInterface& _DB, char const* _KeyColumn,
                                          TValue const& _KeyValue )
{
    auto stmt = DB::Select( TEntity::Table ).Key( _KeyColumn ).Prepare( _DB );
    CHECK_RESULT_RETURN_ERROR( stmt.Code );
    CHECK_RESULT_RETURN_ERROR( stmt.Value->BindValue( _KeyColumn, _KeyValue ) ); // $ + KeyColumn?
    return stmt.Value->ExecuteList<TEntity>();
}

template <class TEntity>
static ResultCode UpdateEntity( DB::DatabaseInterface& _DB, TEntity& _Entity )
{
    if ( _Entity.Id == -1 )
    {
        return ResultCode::InvalidEntityId;
    }

    auto stmt = DB::UpdateEntity( TEntity::Table, _Entity ).Key( "Id" ).Prepare( _DB );
    CHECK_RESULT_RETURN_ERROR( stmt.Code );
    CHECK_RESULT_RETURN_ERROR( StoreEntity( _Entity, *stmt.Value ) );
    CHECK_RESULT_RETURN_ERROR( stmt.Value->BindValue( "$Id", _Entity.Id ) );
    return stmt.Value->Step();
}

template <class TEntity>
static ResultCode DeleteEntity( DB::DatabaseInterface& _DB, TEntity& _Entity )
{
    if ( _Entity.Id == -1 )
    {
        return ResultCode::InvalidEntityId;
    }

    auto stmt = DB::Delete( TEntity::Table ).Key( "Id" ).Prepare( _DB );
    CHECK_RESULT_RETURN_ERROR( stmt.Code );
    CHECK_RESULT_RETURN_ERROR( stmt.Value->BindValue( "$Id", _Entity.Id ) );
    CHECK_RESULT_RETURN_ERROR( stmt.Value->Step() );
    _Entity.Id = -1;
    return ResultCode::OK;
}

Booru::~Booru() {}

ResultCode Booru::OpenDatabase( char const* _Path )
{
    auto db = DB::Sqlite3::DatabaseSqlite3::OpenDatabase( _Path );
    if ( !db )
    {
        return db.Code;
    }

    m_DB = std::move( db.Value );

    auto dbVersion = GetConfigInt64( "db.version" );
    if ( !dbVersion )
    {
        CHECK_RESULT_RETURN_ERROR( CreateTables() );
        dbVersion = GetConfigInt64( "db.version" );
        CHECK_RESULT_RETURN_ERROR( dbVersion.Code );
    }

    for ( INTEGER i = dbVersion.Value; i < k_SQLSchemaVersion; i++ )
    {
        CHECK_RESULT_RETURN_ERROR( UpdateTables( i ) );
    }

    return ResultCode::OK;
}

DB::DatabaseInterface& Booru::GetDatabase() { return *this->m_DB; }

Expected<TEXT> Booru::GetConfig( TEXT const& _Name )
{
    auto stmt = DB::Select( "Config" ).Column( "Value" ).Key( "Name" ).Prepare( *m_DB );
    CHECK_RESULT_RETURN_ERROR( stmt.Code );
    CHECK_RESULT_RETURN_ERROR( stmt.Value->BindValue( "Name", _Name ) );
    return stmt.Value->ExecuteScalar<TEXT>();
}

Expected<INTEGER> Booru::GetConfigInt64( TEXT const& _Key )
{
    auto config = GetConfig( _Key );
    std::cout << _Key << "=" << config.Value << "\n";
    if ( config )
    {
        return ::atol( config.Value.c_str() );
    }
    return config.Code;
}

ResultCode Booru::CreateTables() { return this->m_DB->ExecuteSQL( g_SQLSchema ); }

ResultCode Booru::UpdateTables( int64_t _Version )
{
    return this->m_DB->ExecuteSQL( SQLGetUpdateSchema( _Version ) );
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// PostTypes
// ////////////////////////////////////////////////////////////////////////////////////////////

ResultCode Booru::CreatePostType( PostType& _PostType ) { return CreateEntity( *m_DB, _PostType ); }

ExpectedList<PostType> Booru::GetPostTypes() { return GetEntities<PostType>( *m_DB ); }

Expected<PostType> Booru::GetPostType( INTEGER _Id )
{
    return GetEntity<PostType>( *m_DB, "Id", _Id );
}

Expected<PostType> Booru::GetPostType( char const* _Name )
{
    return GetEntity<PostType>( *m_DB, "Name", _Name );
}

ResultCode Booru::UpdatePostType( PostType& _PostType ) { return UpdateEntity( *m_DB, _PostType ); }

ResultCode Booru::DeletePostType( PostType& _PostType ) { return DeleteEntity( *m_DB, _PostType ); }

// ////////////////////////////////////////////////////////////////////////////////////////////
// Posts
// ////////////////////////////////////////////////////////////////////////////////////////////

ResultCode Booru::CreatePost( Post& _Post ) { return CreateEntity( *m_DB, _Post ); }

ExpectedList<Post> Booru::GetPosts() { return GetEntities<Post>( *m_DB ); }

Expected<Post> Booru::GetPost( INTEGER _Id ) { return GetEntity<Post>( *m_DB, "Id", _Id ); }

Expected<Post> Booru::GetPost( BLOB<16> _MD5 ) { return GetEntity<Post>( *m_DB, "MD5Sum", _MD5 ); }

ResultCode Booru::UpdatePost( Post& _Post ) { return UpdateEntity( *m_DB, _Post ); }

ResultCode Booru::DeletePost( Post& _Post ) { return DeleteEntity( *m_DB, _Post ); }

ResultCode Booru::AddTagToPost( INTEGER _PostId, char const* _Tag )
{
    bool remove = _Tag[0] == '-';
    if ( remove )
    {
        _Tag++;
    }

    Expected<Tag> tag = GetTag( _Tag );
    CHECK_RESULT_RETURN_ERROR_PRINT( tag );

    PostTag postTag;
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
        CHECK_RESULT_RETURN_ERROR_PRINT( tag );
    }

    DB::TransactionGuard transactionGuard( *m_DB );

    // actually add tag
    auto createPostTagResult = CreatePostTag( postTag );
    if (createPostTagResult != ResultCode::AlreadyExists )
    {
        CHECK_RESULT_RETURN_ERROR_PRINT( createPostTagResult );
    }

    // try to honor implications, ignore errors
    ExpectedList<TagImplication> implications = GetTagImplicationsForTag( tag.Value.Id );
    CHECK_RESULT_RETURN_ERROR_PRINT( implications );
    for ( TagImplication const& implication : implications.Value )
    {
        INTEGER impliedTagId = implication.ImpliedTagId;
        if ( implication.Flags & TagImplication::FLAG_REMOVE_TAG )
        {
            PostTag postTag;
            postTag.PostId = _PostId;
            postTag.TagId  = tag.Value.Id;
            CHECK_RESULT_CONTINUE_ERROR_PRINT( DeletePostTag( postTag ) );
        }
        else
        {
            Expected<Tag> impliedTag = GetTag( impliedTagId );
            CHECK_RESULT_CONTINUE_ERROR_PRINT( impliedTag );

            fprintf(stderr, "%ld -> %ld\n", tag.Value.Id, impliedTagId );

            // recurse, added tag may have redirection, implications, ...
            CHECK_RESULT_CONTINUE_ERROR_PRINT( AddTagToPost( _PostId, impliedTag.Value.Name.c_str() ) );
        }
    }

    transactionGuard.Commit();
    return ResultCode::OK;
}

ExpectedList<Post> Booru::FindPosts( char const* _QueryString )
{
    std::vector<String> queryStringTokens = SplitString( _QueryString );
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

    SQL += JoinString( conditionStrings, " AND \n" );
    fprintf( stderr, "%s\n", SQL.c_str() );

    auto stmt = m_DB->PrepareStatement( SQL.c_str() );
    CHECK_RESULT_RETURN_ERROR( stmt.Code );
    return stmt.Value->ExecuteList<Post>();
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// PostTags
// ////////////////////////////////////////////////////////////////////////////////////////////

ResultCode Booru::CreatePostTag( PostTag& _PostTag )
{
    auto stmt = DB::Insert( "PostTags" ).Column( "PostId" ).Column( "TagId" ).Prepare( *m_DB );
    CHECK_RESULT_RETURN_ERROR( stmt.Code );
    CHECK_RESULT_RETURN_ERROR( stmt.Value->BindValue( "PostId", _PostTag.PostId ) );
    CHECK_RESULT_RETURN_ERROR( stmt.Value->BindValue( "TagId", _PostTag.TagId ) );
    return stmt.Value->Step();
}

ExpectedList<PostTag> Booru::GetPostTags() { return GetEntities<PostTag>( *m_DB ); }

ResultCode Booru::DeletePostTag( PostTag& _PostTag )
{
    auto stmt = DB::Delete( "PostTags" ).Key( "PostId" ).Key( "TagId" ).Prepare( *m_DB );
    CHECK_RESULT_RETURN_ERROR( stmt.Code );
    CHECK_RESULT_RETURN_ERROR( stmt.Value->BindValue( "PostId", _PostTag.PostId ) );
    CHECK_RESULT_RETURN_ERROR( stmt.Value->BindValue( "TagId", _PostTag.TagId ) );
    return stmt.Value->Step();
}

ExpectedList<Tag> Booru::GetTagsForPost( INTEGER _PostId )
{
    auto stmt = m_DB->PrepareStatement( R"SQL(
        SELECT * FROM Tags WHERE Tags.Id IN
        ( SELECT TagId FROM PostTags WHERE PostTags.PostId = $PostId ) 
    )SQL" );
    CHECK_RESULT_RETURN_ERROR( stmt.Code );
    CHECK_RESULT_RETURN_ERROR( stmt.Value->BindValue( "$PostId", _PostId ) );
    return stmt.Value->ExecuteList<Tag>();
}

ExpectedList<Post> Booru::GetPostsForTag( INTEGER _TagId )
{
    auto stmt = m_DB->PrepareStatement( R"SQL(
        SELECT * FROM Posts WHERE Posts.Id IN
        ( SELECT PostId FROM PostTags WHERE PostTags.TagId = $TagId ) 
    )SQL" );
    CHECK_RESULT_RETURN_ERROR( stmt.Code );
    CHECK_RESULT_RETURN_ERROR( stmt.Value->BindValue( "$TagId", _TagId ) );
    return stmt.Value->ExecuteList<Post>();
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// PostFiles
// ////////////////////////////////////////////////////////////////////////////////////////////

ResultCode Booru::CreatePostFile( PostFile& _PostFile ) { return CreateEntity( *m_DB, _PostFile ); }

ExpectedList<PostFile> Booru::GetPostFiles() { return GetEntities<PostFile>( *m_DB ); }

ExpectedList<PostFile> Booru::GetFilesForPost( INTEGER _PostId )
{
    return GetEntities<PostFile>( *m_DB, "PostId", _PostId );
}

ResultCode Booru::DeletePostFile( PostFile& _PostFile ) { return DeleteEntity( *m_DB, _PostFile ); }

// ////////////////////////////////////////////////////////////////////////////////////////////
// PostSiteId
// ////////////////////////////////////////////////////////////////////////////////////////////

ResultCode Booru::CreatePostSiteId( PostSiteId& _PostSiteId )
{
    return CreateEntity( *m_DB, _PostSiteId );
}

ExpectedList<PostSiteId> Booru::GetPostSiteIds() { return GetEntities<PostSiteId>( *m_DB ); }

ExpectedList<PostSiteId> Booru::GetPostSiteIdsForPost( INTEGER _PostId )
{
    return GetEntities<PostSiteId>( *m_DB, "PostId", _PostId );
}

ResultCode Booru::DeletePostSiteId( PostSiteId& _PostSiteId )
{
    return DeleteEntity( *m_DB, _PostSiteId );
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// Sites
// ////////////////////////////////////////////////////////////////////////////////////////////

ResultCode Booru::CreateSite( Site& _Site ) { return CreateEntity( *m_DB, _Site ); }

Expected<Site> Booru::GetSite( INTEGER _Id ) { return GetEntity<Site>( *m_DB, "Id", _Id ); }

Expected<Site> Booru::GetSite( char const* _Name )
{
    return GetEntity<Site>( *m_DB, "Name", _Name );
}

ResultCode Booru::UpdateSite( Site& _Site ) { return UpdateEntity( *m_DB, _Site ); }

ResultCode Booru::DeleteSite( Site& _Site ) { return DeleteEntity( *m_DB, _Site ); }

// ////////////////////////////////////////////////////////////////////////////////////////////
// Tags
// ////////////////////////////////////////////////////////////////////////////////////////////

ResultCode Booru::CreateTag( Tag& _Tag ) { return CreateEntity( *m_DB, _Tag ); }

ExpectedList<Tag> Booru::GetTags() { return GetEntities<Tag>( *m_DB ); }

Expected<Tag> Booru::GetTag( INTEGER _Id ) { return GetEntity<Tag>( *m_DB, "Id", _Id ); }

Expected<Tag> Booru::GetTag( char const* _Name ) { return GetEntity<Tag>( *m_DB, "Name", _Name ); }

ResultCode Booru::UpdateTag( Tag& _Tag ) { return UpdateEntity( *m_DB, _Tag ); }

ResultCode Booru::DeleteTag( Tag& _Tag ) { return DeleteEntity( *m_DB, _Tag ); }

ExpectedList<Tag> Booru::MatchTags( TEXT& _Pattern )
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
    CHECK_RESULT_RETURN_ERROR( stmt.Code );
    CHECK_RESULT_RETURN_ERROR( stmt.Value->BindValue( "$Pattern", pattern ) );
    return stmt.Value->ExecuteList<Tag>();
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// TagImplications
// ////////////////////////////////////////////////////////////////////////////////////////////

ResultCode Booru::CreateTagImplication( TagImplication& _TagImplication )
{
    if (_TagImplication.TagId == _TagImplication.ImpliedTagId)
    {
        return ResultCode::InvalidArgument;
    }
    return CreateEntity( *m_DB, _TagImplication );
}

ExpectedList<TagImplication> Booru::GetTagImplications()
{
    return GetEntities<TagImplication>( *m_DB );
}

ExpectedList<TagImplication> Booru::GetTagImplicationsForTag( INTEGER _TagId )
{
    return GetEntities<TagImplication>( *m_DB, "TagId", _TagId );
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// TagTypes
// ////////////////////////////////////////////////////////////////////////////////////////////

ResultCode Booru::CreateTagType( TagType& _TagType ) { return CreateEntity( *m_DB, _TagType ); }

ExpectedList<TagType> Booru::GetTagTypes() { return GetEntities<TagType>( *m_DB ); }

Expected<TagType> Booru::GetTagType( INTEGER _Id )
{
    return GetEntity<TagType>( *m_DB, "Id", _Id );
}

Expected<TagType> Booru::GetTagType( char const* _Name )
{
    return GetEntity<TagType>( *m_DB, "Name", _Name );
}

ResultCode Booru::UpdateTagType( TagType& _TagType ) { return UpdateEntity( *m_DB, _TagType ); }

ResultCode Booru::DeleteTagType( TagType& _TagType ) { return DeleteEntity( *m_DB, _TagType ); }

// ////////////////////////////////////////////////////////////////////////////////////////////
// Utilities
// ////////////////////////////////////////////////////////////////////////////////////////////

Expected<String> Booru::GetSQLConditionForTag( String _Tag )
{
    using namespace std::literals;

    if ( _Tag[0] == '-' )
    {
        Expected<String> subCondition = GetSQLConditionForTag( _Tag.substr( 1 ) );
        CHECK_RESULT_RETURN_ERROR( subCondition );
        return "NOT " + subCondition.Value;
    }

    String lowerTag = _Tag;
    std::transform( begin( lowerTag ), end( lowerTag ), begin( lowerTag ),
                    []( char c ) { return std::tolower( c ); } );

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

    ExpectedList<Tag> tags = MatchTags( _Tag );
    CHECK_RESULT_RETURN_ERROR( tags.Code );

    return "( "
           "   ( "
           "       SELECT  COUNT(*) "
           "       FROM    PostTags "
           "       WHERE   PostTags.PostId =   Posts.Id "
           "       AND     PostTags.TagId  IN  (" +
           JoinString( CollectIds( tags.Value ) ) +
           ") "
           "   ) > 0 "
           ") ";
}

} // namespace Booru
