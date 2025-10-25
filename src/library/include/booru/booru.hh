#pragma once

#include <booru/db/entity.hh>
#include <booru/common.hh>

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
        ResultCode OpenDatabase(StringView const &_Path, bool _Create = false);

        /// @brief Close database connection. Actively running transactions are rolled back.
        void CloseDatabase();

        /// @brief Get interface to the currently open database.
        DB::ExpectedDB GetDatabase();

        // ////////////////////////////////////////////////////////////////////////////////////////////
        // Management
        // ////////////////////////////////////////////////////////////////////////////////////////////

        /// @brief Get an entry from the config table in the database
        /// @param _Name Name of the configuration entry
        /// @return Value of the configuration entry as a TEXT
        Expected<DB::TEXT> GetConfig(DB::TEXT const &_Name);

        /// @brief Set an entry in the config table in the database.
        /// Updates existing entry by that name or creates it.
        /// @param _Name Name of the configuration entry
        /// @param _Value Value of the configuration entry as a TEXT
        ResultCode SetConfig(DB::TEXT const &_Name, DB::TEXT const &_Value);

        /// @brief Helper function to get a config entry as an INTEGER
        /// @param _Name Name of the configuration entry
        Expected<DB::INTEGER> GetConfigInt64(DB::TEXT const &_Name);

        // ////////////////////////////////////////////////////////////////////////////////////////////
        // Entities common functions
        // ////////////////////////////////////////////////////////////////////////////////////////////

        /// @brief Create a new entity in database.
        template <class TEntity>
        ResultCode Create(TEntity &_Entity);

        /// @brief Update an new entity in database.
        template <class TEntity>
        ResultCode Update(TEntity &_Entity);

        /// @brief Delete entity from database.
        template <class TEntity>
        ResultCode Delete(TEntity &_Entity);

        // ////////////////////////////////////////////////////////////////////////////////////////////
        // PostTypes
        // ////////////////////////////////////////////////////////////////////////////////////////////

        /// @brief Get all post types
        ExpectedVector<DB::Entities::PostType> GetPostTypes();

        /// @brief Get post type by Id
        Expected<DB::Entities::PostType> GetPostType(DB::INTEGER _Id);

        /// @brief Get post type by name
        Expected<DB::Entities::PostType> GetPostType(DB::TEXT const &_Name);

        // ////////////////////////////////////////////////////////////////////////////////////////////
        // Posts
        // ////////////////////////////////////////////////////////////////////////////////////////////

        /// @brief Get all posts.
        ExpectedVector<DB::Entities::Post> GetPosts();

        /// @brief Get post by Id.
        Expected<DB::Entities::Post> GetPost(DB::INTEGER _Id);

        /// @brief Get post by hash.
        Expected<DB::Entities::Post> GetPost(DB::BLOB<16> _Id);

        /// @brief Add a tag by name to a post. Considers negation, redirections and implications.
        ResultCode AddTagToPost(DB::INTEGER _PostId, StringView const &_Tag);

        /// @brief Remove a tag by Id from a post .
        ResultCode RemoveTagFromPost(DB::INTEGER _PostId, DB::INTEGER _Tag);

        /// @brief Get all posts that match a given query.
        ExpectedVector<DB::Entities::Post> FindPosts(StringView const &_QueryString);

        // ////////////////////////////////////////////////////////////////////////////////////////////
        // PostTags
        // ////////////////////////////////////////////////////////////////////////////////////////////

        /// @brief Get all post/tag associations.
        ExpectedVector<DB::Entities::PostTag> GetPostTags();

        /// @brief Get all tags for a post by Id.
        ExpectedVector<DB::Entities::Tag> GetTagsForPost(DB::INTEGER _PostId);

        /// @brief Get all posts for a tag by Id.
        ExpectedVector<DB::Entities::Post> GetPostsForTag(DB::INTEGER _TagId);

        /// @brief Find a post/tag associations.
        Expected<DB::Entities::PostTag> FindPostTag(DB::INTEGER _PostId, DB::INTEGER _Tag);

        // ////////////////////////////////////////////////////////////////////////////////////////////
        // PostFiles
        // ////////////////////////////////////////////////////////////////////////////////////////////

        /// @brief Get all post files.
        ExpectedVector<DB::Entities::PostFile> GetPostFiles();

        /// @brief Get all files for a given post by Id.
        ExpectedVector<DB::Entities::PostFile> GetFilesForPost(DB::INTEGER _PostId);

        // ////////////////////////////////////////////////////////////////////////////////////////////
        // PostSiteId
        // ////////////////////////////////////////////////////////////////////////////////////////////

        /// @brief Get all post site ids.
        ExpectedVector<DB::Entities::PostSiteId> GetPostSiteIds();

        /// @brief Get all site ids for a given post.
        ExpectedVector<DB::Entities::PostSiteId> GetPostSiteIdsForPost(DB::INTEGER _PostId);

        // ////////////////////////////////////////////////////////////////////////////////////////////
        // Sites
        // ////////////////////////////////////////////////////////////////////////////////////////////

        /// @brief Get all sites.
        // TODO: GetSites();

        /// @brief Get all site by id.
        Expected<DB::Entities::Site> GetSite(DB::INTEGER _Id);

        /// @brief Get all site by name.
        Expected<DB::Entities::Site> GetSite(DB::TEXT const &_Name);

        // ////////////////////////////////////////////////////////////////////////////////////////////
        // Tags
        // ////////////////////////////////////////////////////////////////////////////////////////////

        /// @brief Get all tags.
        ExpectedVector<DB::Entities::Tag> GetTags();

        /// @brief Get tag by id.
        Expected<DB::Entities::Tag> GetTag(DB::INTEGER _Id);

        /// @brief Get all tags by name.
        Expected<DB::Entities::Tag> GetTag(DB::TEXT const &_Name);

        /// @brief Get all tags that match a given pattern.
        ExpectedVector<DB::Entities::Tag> MatchTags(StringView const &_Pattern);

        // ////////////////////////////////////////////////////////////////////////////////////////////
        // TagImplications
        // ////////////////////////////////////////////////////////////////////////////////////////////

        /// @brief Get all tag implications.
        ExpectedVector<DB::Entities::TagImplication> GetTagImplications();

        /// @brief Get tag implication by tag id.
        ExpectedVector<DB::Entities::TagImplication> GetTagImplicationsForTag(DB::INTEGER _TagId);

        // ////////////////////////////////////////////////////////////////////////////////////////////
        // TagTypes
        // ////////////////////////////////////////////////////////////////////////////////////////////

        /// @brief Get all tag types.
        ExpectedVector<DB::Entities::TagType> GetTagTypes();

        /// @brief Get tag type by Id.
        Expected<DB::Entities::TagType> GetTagType(DB::INTEGER _Id);

        /// @brief Get tag type by name.
        Expected<DB::Entities::TagType> GetTagType(DB::TEXT const &_Name);

        // ////////////////////////////////////////////////////////////////////////////////////////////
        // Utilities
        // ////////////////////////////////////////////////////////////////////////////////////////////

        /// @brief Convert a tag specification into a SQL condition for use in WHERE clauses.
        /// Matches wildcards and negations.
        Expected<String> GetSQLConditionForTag(StringView const &_Tag);

    private:
        Booru();

        /// @brief Create database tables.
        ResultCode CreateTables();

        /// @brief Update database table to ne schema version.
        ResultCode UpdateTables(int64_t _Version);

        /// Database handle
        DB::DBPtr m_DB;
    };

    template <class TEntity>
    ResultCode Booru::Create(TEntity &_Entity)
    {
        CHECK_RETURN_RESULT_ON_ERROR(_Entity.CheckValidForCreate());
        CHECK_RETURN_RESULT_ON_ERROR(_Entity.CheckValues());
        return GetDatabase()
            .Then(DB::Entities::Create<TEntity>, _Entity);
    }

    /// @brief Update an new entity in database.
    template <class TEntity>
    ResultCode Booru::Update(TEntity &_Entity)
    {
        CHECK_RETURN_RESULT_ON_ERROR(_Entity.CheckValidForUpdate());
        CHECK_RETURN_RESULT_ON_ERROR(_Entity.CheckValues());
        return GetDatabase()
            .Then(DB::Entities::Update<TEntity>, _Entity);
    }

    /// @brief Delete entity from database.
    template <class TEntity>
    ResultCode Booru::Delete(TEntity &_Entity)
    {
        CHECK_RETURN_RESULT_ON_ERROR(_Entity.CheckValidForDelete());
        return GetDatabase()
            .Then(DB::Entities::Delete<TEntity>, _Entity);
    }

} // namespace Booru
