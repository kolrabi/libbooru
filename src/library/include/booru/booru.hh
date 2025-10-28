#pragma once

#include <booru/db/entities.hh>

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
    ResultCode OpenDatabase(StringView const& _Path, bool _Create = false);

    /// @brief Close database connection. Actively running transactions are
    /// rolled back.
    void CloseDatabase();

    /// @brief Get interface to the currently open database.
    DB::ExpectedDB GetDatabase();

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // Management
    // ////////////////////////////////////////////////////////////////////////////////////////////

    /// @brief Get an entry from the config table in the database
    /// @param _Name Name of the configuration entry
    /// @return Value of the configuration entry as a TEXT
    Expected<DB::TEXT> GetConfig(DB::TEXT const& _Name);

    /// @brief Set an entry in the config table in the database.
    /// Updates existing entry by that name or creates it.
    /// @param _Name Name of the configuration entry
    /// @param _Value Value of the configuration entry as a TEXT
    ResultCode SetConfig(DB::TEXT const& _Name, DB::TEXT const& _Value);

    /// @brief Helper function to get a config entry as an INTEGER
    /// @param _Name Name of the configuration entry
    Expected<DB::INTEGER> GetConfigInt64(DB::TEXT const& _Name);

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // Entities common functions
    // ////////////////////////////////////////////////////////////////////////////////////////////

    /// @brief Create a new entity in database.
    template <class TEntity> Expected<TEntity> Create(TEntity& _Entity);

    /// @brief Get all entities
    template <class TEntity> ExpectedVector<TEntity> GetAll();

    /// @brief Get all matching entities
    template <class TEntity, class TKey>
    ExpectedVector<TEntity> GetAll(StringView const& _Key, TKey const& _Value);

    /// @brief Get one matching entities
    template <class TEntity> Expected<TEntity> Get(DB::INTEGER _Id);

    /// @brief Get one matching entities
    template <class TEntity, class TKey>
    Expected<TEntity> Get(StringView const& _Key, TKey const& _Value);

    /// @brief Update an new entity in database.
    template <class TEntity> Expected<TEntity> Update(TEntity& _Entity);

    /// @brief Delete entity from database.
    template <class TEntity> Expected<TEntity> Delete(TEntity& _Entity);

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // PostTypes
    // ////////////////////////////////////////////////////////////////////////////////////////////

    ExpectedVector<DB::Entities::PostType> GetPostTypes();
    Expected<DB::Entities::PostType> GetPostType(DB::INTEGER _Id);
    Expected<DB::Entities::PostType> GetPostType(DB::TEXT const& _Name);

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // Posts
    // ////////////////////////////////////////////////////////////////////////////////////////////

    ExpectedVector<DB::Entities::Post> GetPosts();
    Expected<DB::Entities::Post> GetPost(DB::INTEGER _Id);
    Expected<DB::Entities::Post> GetPost(DB::BLOB<16> _Id);
    Expected<DB::Entities::Post> AddTagToPost(DB::Entities::Post const& _Post,
                                              DB::Entities::Tag const& _TagId);
    Expected<DB::Entities::Post>
    RemoveTagFromPost(DB::Entities::Post const& _Post,
                      DB::Entities::Tag const& _Tag);
    ExpectedVector<DB::Entities::Post>
    FindPosts(StringView const& _QueryString);

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // PostTags
    // ////////////////////////////////////////////////////////////////////////////////////////////

    ExpectedVector<DB::Entities::PostTag> GetPostTags();
    ExpectedVector<DB::Entities::Tag> GetTagsForPost(DB::INTEGER _PostId);
    ExpectedVector<DB::Entities::Post> GetPostsForTag(DB::INTEGER _TagId);
    Expected<DB::Entities::PostTag> FindPostTag(DB::INTEGER _PostId,
                                                DB::INTEGER _Tag);

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // PostFiles
    // ////////////////////////////////////////////////////////////////////////////////////////////

    ExpectedVector<DB::Entities::PostFile> GetPostFiles();
    ExpectedVector<DB::Entities::PostFile> GetFilesForPost(DB::INTEGER _PostId);

    // TODO: AddFileToPost
    // TODO: GetPostForFile

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // Sites
    // ////////////////////////////////////////////////////////////////////////////////////////////

    ExpectedVector<DB::Entities::Site> GetSites();
    Expected<DB::Entities::Site> GetSite(DB::INTEGER _Id);
    Expected<DB::Entities::Site> GetSite(DB::TEXT const& _Name);

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // Tags
    // ////////////////////////////////////////////////////////////////////////////////////////////

    ExpectedVector<DB::Entities::Tag> GetTags();
    Expected<DB::Entities::Tag> GetTag(DB::INTEGER _Id);
    Expected<DB::Entities::Tag> GetTag(DB::TEXT const& _Name);
    ExpectedVector<DB::Entities::Tag> MatchTags(StringView const& _Pattern);
    Expected<DB::Entities::Tag>
    FollowRedirections(DB::Entities::Tag const& _Tag);

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // TagImplications
    // ////////////////////////////////////////////////////////////////////////////////////////////

    /// @brief Get all tag implications.
    ExpectedVector<DB::Entities::TagImplication> GetTagImplications();

    /// @brief Get tag implication by tag id.
    ExpectedVector<DB::Entities::TagImplication>
    GetTagImplicationsForTag(DB::Entities::Tag const& _TagId);

    // TODO: AddPositiveImplicationToTag(Tag, Tag)
    // TODO: AddNegativeImplicationToTag(Tag, Tag)
    // TODO: RemoveImplicationFromTag(Tag, Tag)

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // TagTypes
    // ////////////////////////////////////////////////////////////////////////////////////////////

    ExpectedVector<DB::Entities::TagType> GetTagTypes();
    Expected<DB::Entities::TagType> GetTagType(DB::INTEGER _Id);
    Expected<DB::Entities::TagType> GetTagType(DB::TEXT const& _Name);

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // Utilities
    // ////////////////////////////////////////////////////////////////////////////////////////////

    /// @brief Convert a tag specification into a SQL condition for use in WHERE
    /// clauses. Matches wildcards and negations.
    Expected<String> GetSQLConditionForTag(StringView const& _Tag);

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
inline Expected<TEntity> Booru::Create(TEntity& _Entity)
{
    CHECK_RETURN_RESULT_ON_ERROR(_Entity.CheckValidForCreate());
    CHECK_RETURN_RESULT_ON_ERROR(_Entity.CheckValues());
    return GetDatabase().Then(DB::Entities::Create<TEntity>, _Entity);
}

template <class TEntity> inline ExpectedVector<TEntity> Booru::GetAll()
{
    return GetDatabase().Then(DB::Entities::GetAll<TEntity>);
}

template <class TEntity, class TKey>
inline ExpectedVector<TEntity> Booru::GetAll(StringView const& _Key,
                                             TKey const& _Value)
{
    return GetDatabase().Then(DB::Entities::GetAllWithKey<TEntity, TKey>, _Key,
                              _Value);
}

template <class TEntity> inline Expected<TEntity> Booru::Get(DB::INTEGER _Id)
{
    return GetDatabase().Then(DB::Entities::GetWithKey<TEntity, DB::INTEGER>,
                              "Id", _Id);
}

template <class TEntity, class TKey>
inline Expected<TEntity> Booru::Get(StringView const& _Key, TKey const& _Value)
{
    return GetDatabase().Then(DB::Entities::GetWithKey<TEntity, TKey>, _Key,
                              _Value);
}

/// @brief Update an new entity in database.
template <class TEntity>
inline Expected<TEntity> Booru::Update(TEntity& _Entity)
{
    CHECK_RETURN_RESULT_ON_ERROR(_Entity.CheckValidForUpdate());
    CHECK_RETURN_RESULT_ON_ERROR(_Entity.CheckValues());
    return GetDatabase()
        .Then(&DB::Entities::Update<TEntity>, _Entity)
        .ThenValue(_Entity);
}

/// @brief Delete entity from database.
template <class TEntity> Expected<TEntity> Booru::Delete(TEntity& _Entity)
{
    CHECK_RETURN_RESULT_ON_ERROR(_Entity.CheckValidForDelete());
    return GetDatabase().Then(&DB::Entities::Delete<TEntity>, _Entity);
}

} // namespace Booru
