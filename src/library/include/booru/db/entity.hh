#pragma once

#include <booru/db.hh>
#include <booru/db/query.hh>
#include <booru/db/visitors.hh>

namespace Booru::DB::Entities
{

static constexpr String LOGGER = "booru.db.entity";

#define ENTITY_PROPERTY( Name ) \
    CHECK_RETURN_RESULT_ON_ERROR( _Visitor.Property( #Name, Name ) )
#define ENTITY_PROPERTY_KEY( Name ) \
    CHECK_RETURN_RESULT_ON_ERROR( _Visitor.Property( #Name, Name, true ) )

/// @brief Base class for all entities.
/// Derived classes must implement an IterateProperties function that takes a visitor object
/// and call its Property() function for each entity property. You can use the ENTITY_PROPERTY
/// and ENTITY_PROPERTY_KEY macros.
template<class TEntity>
struct Entity
{
    INTEGER Id = -1; // primary key
};

template<class TEntity>
std::ostream & operator<<(std::ostream & _Stream, Entity<TEntity> const & _Entity )
{
    return _Stream << EntityToString( static_cast<TEntity const &>(_Entity) );
}

/// @brief Store properties from a statement into an entity.
template <class TValue>
ResultCode LoadEntity( TValue& _Entity, DatabasePreparedStatementInterface& _Stmt )
{
    auto Visitor = Visitors::LoadFromStatementPropertyVisitor( _Stmt );
    return _Entity.IterateProperties( Visitor );
}

/// @brief Store entity properties into a statement.
template <class TValue>
ResultCode StoreEntity( TValue& _Entity, DatabasePreparedStatementInterface& _Stmt )
{
    auto Visitor = Visitors::StoreToStatementPropertyVisitor( _Stmt );
    return _Entity.IterateProperties( Visitor );
}

/// @brief Get a vector will all the ids of a collection of entities.
template <class TEntity>
Vector<DB::INTEGER> CollectIds( Vector<TEntity> const& _Entities )
{
    Vector<DB::INTEGER> Ids;
    for ( auto const& entity : _Entities )
    {
        Ids.push_back( entity.Id );
    }
    return Ids;
}

/// @brief Create a new entity in the database using values from the provided entity.
template <class TEntity>
ResultCode Create( DB::DatabaseInterface& _DB, TEntity& _Entity )
{
    auto stmt = DB::Query::InsertEntity<TEntity>( TEntity::Table, _Entity ).Prepare( _DB );
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Code );
    CHECK_RETURN_RESULT_ON_ERROR( StoreEntity( _Entity, *stmt.Value ) );
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Value->Step() );
    return _DB.GetLastRowId( _Entity.Id );
}

/// @brief Try to retrieve a single entity from the database matching the given key in the given column. Only one entity will be returned. If no entity is found, an error will be returned.
template <class TEntity, class TValue>
Expected<TEntity> GetWithKey( DB::DatabaseInterface& _DB, StringView const & _KeyColumn, TValue const& _KeyValue )
{
    auto stmt = DB::Query::Select( TEntity::Table ).Key( _KeyColumn ).Prepare( _DB );
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Code );
    CHECK_RETURN_RESULT_ON_ERROR(
        stmt.Value->BindValue( _KeyColumn, _KeyValue ) );
    return stmt.Value->ExecuteRow<TEntity>( true );
}

/// @brief Retrieve all entities of a given type from the database. If no entities are found, an empty list will be returned. On error an error code is returned.
template <class TEntity>
ExpectedVector<TEntity> GetAll( DB::DatabaseInterface& _DB )
{
    auto stmt = DB::Query::Select( TEntity::Table ).Prepare( _DB );
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Code );
    return stmt.Value->ExecuteList<TEntity>();
}

/// @brief Retrieve all entities of a given type from the database that match the given key in the given column. If no entities are found, an empty list will be returned. On error an error code is returned.
template <class TEntity, class TValue>
ExpectedVector<TEntity> GetAllWithKey( DB::DatabaseInterface& _DB, StringView const & _KeyColumn,
                                          TValue const& _KeyValue )
{
    auto stmt = DB::Query::Select( TEntity::Table ).Key( _KeyColumn ).Prepare( _DB );
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Code );
    CHECK_RETURN_RESULT_ON_ERROR(
        stmt.Value->BindValue( _KeyColumn, _KeyValue ) );
    return stmt.Value->ExecuteList<TEntity>();
}

/// @brief Update the values of an entity in the database. The entity must have a valid ID set or an error code will be returned.
template <class TEntity>
ResultCode Update( DB::DatabaseInterface& _DB, TEntity& _Entity )
{
    auto stmt = DB::Query::UpdateEntity( TEntity::Table, _Entity ).Key( "Id" ).Prepare( _DB );
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Code );
    CHECK_RETURN_RESULT_ON_ERROR( StoreEntity( _Entity, *stmt.Value ) );
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Value->BindValue( "Id", _Entity.Id ) );
    return stmt.Value->Step(true);
}

/// @brief Delete an entity from the database. The entity must have a valid ID set. 
template <class TEntity>
static ResultCode Delete( DB::DatabaseInterface& _DB, TEntity& _Entity )
{
    auto stmt = DB::Query::Delete( TEntity::Table ).Key( "Id" ).Prepare( _DB );
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Code );
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Value->BindValue( "Id", _Entity.Id ) );
    CHECK_RETURN_RESULT_ON_ERROR( stmt.Value->Step(true) );
    _Entity.Id = -1;
    return ResultCode::OK;
}

// forward declaration of entities

struct PostFile;
struct PostSiteId;
struct PostTag;
struct PostType;
struct Post;
struct Site;
struct TagImplication;
struct TagType;
struct Tag;

// A post can be suited for different audiences, such as adults or children. The rating of a post can be used to determine whether it should be displayed.
enum
{
    RATING_UNRATED      = 0,
    RATING_GENERAL      = 1,
    RATING_SENSITIVE    = 2,
    RATING_QUESTIONABLE = 3,
    RATING_EXPLICIT     = 4
};

/// (Debug tool) Convert an entity to a printable string.
template <class TEntity>
static inline String EntityToString( TEntity const & _Item )
{
    Visitors::ToStringPropertyVisitor Visitor;
    auto result = const_cast<TEntity&>(_Item).IterateProperties( Visitor );
    return Visitor.m_String;
}

} // namespace Booru::DB::Entities
