#pragma once

#include <booru/db/entity.hh>
#include <booru/common.hh>

class DatabaseInterface;

namespace Booru
{

class Booru
{
  static inline constexpr String LOGGER = "booru";

  public:

    /// @brief Create and initialize a instance of the library.
    static Owning<Booru> InitializeLibrary();

    /// @brief Get currently supported version of the database schema.
    static int64_t GetSchemaVersion();

    /// @brief Public d'tor.
    ~Booru();

    /// @brief Open database connection.
    /// @param _Path Connection string, eg. file path for sqlite.
    /// @param _Create If true, create and database tables if they are not found
    ResultCode OpenDatabase( StringView const & _Path, bool _Create = false );

    /// @brief Close database connection. Actively running transactions are rolled back.
    void CloseDatabase();

    /// @brief Get interface to the currently open database.
    Expected<DB::DatabaseInterface*> GetDatabase();

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // Management
    // ////////////////////////////////////////////////////////////////////////////////////////////

    /// @brief Get an entry from the config table in the database
    /// @param _Name Name of the configuration entry
    /// @return Value of the configuration entry as a TEXT
    Expected<DB::TEXT> GetConfig( DB::TEXT const & _Name );

    /// @brief Set an entry in the config table in the database. 
    /// Updates existing entry by that name or creates it.
    /// @param _Name Name of the configuration entry
    /// @param _Value Value of the configuration entry as a TEXT
    ResultCode SetConfig( DB::TEXT const & _Name, DB::TEXT const & _Value );

    /// @brief Helper function to get a config entry as an INTEGER
    /// @param _Name Name of the configuration entry
    Expected<DB::INTEGER> GetConfigInt64( DB::TEXT const & _Name );

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // Entities common functions
    // ////////////////////////////////////////////////////////////////////////////////////////////

    /// @brief Create a new entity in database.
    template<class TEntity>
    ResultCode Create( TEntity& _Entity ) 
    { 
        CHECK_RETURN_RESULT_ON_ERROR( _Entity.CheckValidForCreate() );
        CHECK_RETURN_RESULT_ON_ERROR( _Entity.CheckValues() );
        CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
        return DB::Entities::Create( *db, _Entity ); 
    }

    /// @brief Update an new entity in database.
    template<class TEntity>
    ResultCode Update( TEntity& _Entity )
    {
        CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
        CHECK_RETURN_RESULT_ON_ERROR( _Entity.CheckValidForUpdate() );
        CHECK_RETURN_RESULT_ON_ERROR( _Entity.CheckValues() );
        return DB::Entities::Update( *db, _Entity );
    }

    /// @brief Delete entity from database.
    template<class TEntity>
    ResultCode Delete( TEntity& _Entity )
    {
        CHECK_VAR_RETURN_RESULT_ON_ERROR( db, GetDatabase() );
        CHECK_RETURN_RESULT_ON_ERROR( _Entity.CheckValidForDelete() );
        return DB::Entities::Delete( *db, _Entity );
    }

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // PostTypes
    // ////////////////////////////////////////////////////////////////////////////////////////////

    /// @brief Get all post types
    ExpectedVector<DB::Entities::PostType> GetPostTypes();

    /// @brief Get post type by Id
    Expected<DB::Entities::PostType> GetPostType( DB::INTEGER _Id );

    /// @brief Get post type by name
    Expected<DB::Entities::PostType> GetPostType( DB::TEXT const & _Name );

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // Posts
    // ////////////////////////////////////////////////////////////////////////////////////////////

    ExpectedVector<DB::Entities::Post> GetPosts();
    Expected<DB::Entities::Post> GetPost( DB::INTEGER _Id );
    Expected<DB::Entities::Post> GetPost( DB::BLOB<16> _Id );

    ResultCode AddTagToPost( DB::INTEGER _PostId, StringView const & _Tag );
    ResultCode RemoveTagFromPost( DB::INTEGER _PostId, DB::INTEGER _Tag );

    ExpectedVector<DB::Entities::Post> FindPosts( StringView const & _QueryString );

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // PostTags
    // ////////////////////////////////////////////////////////////////////////////////////////////

    ExpectedVector<DB::Entities::PostTag> GetPostTags();
    ExpectedVector<DB::Entities::Tag> GetTagsForPost( DB::INTEGER _PostId );
    ExpectedVector<DB::Entities::Post> GetPostsForTag( DB::INTEGER _TagId );
    Expected<DB::Entities::PostTag> FindPostTag( DB::INTEGER _PostId, DB::INTEGER _Tag );

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // PostFiles
    // ////////////////////////////////////////////////////////////////////////////////////////////

    ExpectedVector<DB::Entities::PostFile> GetPostFiles();
    ExpectedVector<DB::Entities::PostFile> GetFilesForPost( DB::INTEGER _PostId );

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // PostSiteId
    // ////////////////////////////////////////////////////////////////////////////////////////////

    ExpectedVector<DB::Entities::PostSiteId> GetPostSiteIds();
    ExpectedVector<DB::Entities::PostSiteId> GetPostSiteIdsForPost( DB::INTEGER _PostId );

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // Sites
    // ////////////////////////////////////////////////////////////////////////////////////////////

    Expected<DB::Entities::Site> GetSite( DB::INTEGER _Id );
    Expected<DB::Entities::Site> GetSite( DB::TEXT const & _Name );

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // Tags
    // ////////////////////////////////////////////////////////////////////////////////////////////

    ExpectedVector<DB::Entities::Tag> GetTags();
    Expected<DB::Entities::Tag> GetTag( DB::INTEGER _Id );
    Expected<DB::Entities::Tag> GetTag( DB::TEXT const & _Name );
    ExpectedVector<DB::Entities::Tag> MatchTags( StringView const & _Pattern );

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // TagImplications
    // ////////////////////////////////////////////////////////////////////////////////////////////

    ExpectedVector<DB::Entities::TagImplication> GetTagImplications();
    ExpectedVector<DB::Entities::TagImplication> GetTagImplicationsForTag( DB::INTEGER _TagId );

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // TagTypes
    // ////////////////////////////////////////////////////////////////////////////////////////////

    ExpectedVector<DB::Entities::TagType> GetTagTypes();
    Expected<DB::Entities::TagType> GetTagType( DB::INTEGER _Id );
    Expected<DB::Entities::TagType> GetTagType( DB::TEXT const & _Name );

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // Utilities
    // ////////////////////////////////////////////////////////////////////////////////////////////

    /// @brief Convert a tag specification into a SQL condition for use in WHERE clauses. 
    /// Matches wildcards and negations.
    Expected<String> GetSQLConditionForTag( StringView const & _Tag );

  private:

    Booru();

    /// @brief Create database tables.
    ResultCode CreateTables();

    /// @brief Update database table to ne schema version.
    ResultCode UpdateTables( int64_t _Version );    

    /// Database handle
    Owning<DB::DatabaseInterface> m_DB;
};

} // namespace Booru
