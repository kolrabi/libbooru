#pragma once

#include "booru/db/entity.hh"
#include "common.hh"
#include "result.hh"

class DatabaseInterface;

namespace Booru
{

class Booru
{
  public:
    static Owning<Booru> InitializeLibrary();

    ~Booru();

    ResultCode OpenDatabase( StringView const & _Path, bool _Create = false );
    void CloseDatabase();
    DB::DatabaseInterface& GetDatabase();

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // Management
    // ////////////////////////////////////////////////////////////////////////////////////////////

    Expected<DB::TEXT> GetConfig( DB::TEXT const & _Key );
    Expected<DB::INTEGER> GetConfigInt64( DB::TEXT const & _Key );
    ResultCode CreateTables();

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // PostTypes
    // ////////////////////////////////////////////////////////////////////////////////////////////

    ResultCode CreatePostType( DB::Entities::PostType& _PostType );
    ExpectedList<DB::Entities::PostType> GetPostTypes();
    Expected<DB::Entities::PostType> GetPostType( DB::INTEGER _Id );
    Expected<DB::Entities::PostType> GetPostType( DB::TEXT const & _Name );
    ResultCode UpdatePostType( DB::Entities::PostType& _PostType );
    ResultCode DeletePostType( DB::Entities::PostType& _PostType );

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // Posts
    // ////////////////////////////////////////////////////////////////////////////////////////////

    ResultCode CreatePost( DB::Entities::Post& _Post );
    ExpectedList<DB::Entities::Post> GetPosts();
    Expected<DB::Entities::Post> GetPost( DB::INTEGER _Id );
    Expected<DB::Entities::Post> GetPost( DB::BLOB<16> _Id );
    ResultCode UpdatePost( DB::Entities::Post& _Post );
    ResultCode DeletePost( DB::Entities::Post& _Post );

    ResultCode AddTagToPost( DB::INTEGER _PostId, StringView const & _Tag );
    ExpectedList<DB::Entities::Post> FindPosts( StringView const & _QueryString );

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // PostTags
    // ////////////////////////////////////////////////////////////////////////////////////////////

    ResultCode CreatePostTag( DB::Entities::PostTag& _PostType );
    ExpectedList<DB::Entities::PostTag> GetPostTags();
    ResultCode DeletePostTag( DB::Entities::PostTag& _PostTag );

    ExpectedList<DB::Entities::Tag> GetTagsForPost( DB::INTEGER _PostId );
    ExpectedList<DB::Entities::Post> GetPostsForTag( DB::INTEGER _TagId );

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // PostFiles
    // ////////////////////////////////////////////////////////////////////////////////////////////

    ResultCode CreatePostFile( DB::Entities::PostFile& _PostFile );
    ExpectedList<DB::Entities::PostFile> GetPostFiles();
    ExpectedList<DB::Entities::PostFile> GetFilesForPost( DB::INTEGER _PostId );
    ResultCode DeletePostFile( DB::Entities::PostFile& _PostFile );

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // PostSiteId
    // ////////////////////////////////////////////////////////////////////////////////////////////

    ResultCode CreatePostSiteId( DB::Entities::PostSiteId& _PostSiteId );
    ExpectedList<DB::Entities::PostSiteId> GetPostSiteIds();
    ExpectedList<DB::Entities::PostSiteId> GetPostSiteIdsForPost( DB::INTEGER _PostId );
    ResultCode DeletePostSiteId( DB::Entities::PostSiteId& _PostSiteId );

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // Sites
    // ////////////////////////////////////////////////////////////////////////////////////////////

    ResultCode CreateSite( DB::Entities::Site& _Site );
    Expected<DB::Entities::Site> GetSite( DB::INTEGER _Id );
    Expected<DB::Entities::Site> GetSite( DB::TEXT const & _Name );
    ResultCode UpdateSite( DB::Entities::Site& _Site );
    ResultCode DeleteSite( DB::Entities::Site& _Site );

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // Tags
    // ////////////////////////////////////////////////////////////////////////////////////////////

    ResultCode CreateTag( DB::Entities::Tag& _Tag );
    ExpectedList<DB::Entities::Tag> GetTags();
    Expected<DB::Entities::Tag> GetTag( DB::INTEGER _Id );
    Expected<DB::Entities::Tag> GetTag( DB::TEXT const & _Name );
    ResultCode UpdateTag( DB::Entities::Tag& _Tag );
    ResultCode DeleteTag( DB::Entities::Tag& _Tag );

    ExpectedList<DB::Entities::Tag> MatchTags( DB::TEXT& _Pattern );

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // TagImplications
    // ////////////////////////////////////////////////////////////////////////////////////////////

    ResultCode CreateTagImplication( DB::Entities::TagImplication& _TagImplication );
    ExpectedList<DB::Entities::TagImplication> GetTagImplications();
    ExpectedList<DB::Entities::TagImplication> GetTagImplicationsForTag( DB::INTEGER _TagId );

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // TagTypes
    // ////////////////////////////////////////////////////////////////////////////////////////////

    ResultCode CreateTagType( DB::Entities::TagType& _TagType );
    ExpectedList<DB::Entities::TagType> GetTagTypes();
    Expected<DB::Entities::TagType> GetTagType( DB::INTEGER _Id );
    Expected<DB::Entities::TagType> GetTagType( DB::TEXT const & _Name );
    ResultCode UpdateTagType( DB::Entities::TagType& _TagType );
    ResultCode DeleteTagType( DB::Entities::TagType& _TagType );

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // Utilities
    // ////////////////////////////////////////////////////////////////////////////////////////////

    Expected<String> GetSQLConditionForTag( String _Tag );

  private:

    Booru();

    ResultCode UpdateTables( int64_t _Version );    
    Owning<DB::DatabaseInterface> m_DB;
};

} // namespace Booru
