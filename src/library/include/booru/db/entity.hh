#pragma once

#include "booru/db/types.hh"
#include "booru/db/visitors.hh"

namespace Booru::DB::Entities
{

#define ENTITY_PROPERTY( Name ) CHECK_RESULT_RETURN_ERROR( _Visitor.Property( #Name, Name ) )
#define ENTITY_PROPERTY_KEY( Name )                                                                \
    CHECK_RESULT_RETURN_ERROR( _Visitor.Property( #Name, Name, true ) )

/// @brief Base class for all entities.
/// Derived classes must implement an IterateProperties function that takes a visitor object
/// and call its Property() function for each entity property. You can use the ENTITY_PROPERTY
/// and ENTITY_PROPERTY_KEY macros.
struct Entity
{
    INTEGER Id = -1; // primary key
};

/// @brief Store properties from a statement into an entity.
template <class TValue>
static inline ResultCode LoadEntity( TValue& _Entity, DatabasePreparedStatementInterface& _Stmt )
{
    auto Visitor = Visitors::LoadFromStatementPropertyVisitor( _Stmt );
    return _Entity.IterateProperties( Visitor );
}

/// @brief Store entity properties into a statement.
template <class TValue>
static inline ResultCode StoreEntity( TValue& _Entity, DatabasePreparedStatementInterface& _Stmt )
{
    auto Visitor = Visitors::StoreToStatementPropertyVisitor( _Stmt );
    return _Entity.IterateProperties( Visitor );
}

template <class TEntity>
static inline std::vector<INTEGER> CollectIds( std::vector<TEntity> const& _Entities )
{
    std::vector<INTEGER> Ids;
    for ( auto const& entity : _Entities )
    {
        Ids.push_back( entity.Id );
    }
    return Ids;
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

enum
{
    RATING_UNRATED      = 0,
    RATING_GENERAL      = 1,
    RATING_SENSITIVE    = 2,
    RATING_QUESTIONABLE = 3,
    RATING_EXPLICIT     = 4
};

} // namespace Booru::DB::Entities

template <class TEntity>
static inline String EntityToString( TEntity& _Item )
{
    Booru::DB::Visitors::ToStringPropertyVisitor Visitor;
    auto result = _Item.IterateProperties( Visitor );
    return Visitor.m_String;
}
