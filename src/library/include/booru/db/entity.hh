#pragma once

#include <booru/db.hh>
#include <booru/db/query.hh>

namespace Booru::DB::Entities
{

static constexpr String LOGGER = "booru.db.entity";

#define ENTITY_PROPERTY(Name)                                                  \
    CHECK_RETURN_RESULT_ON_ERROR(_Visitor.Property(#Name, Name))
#define ENTITY_PROPERTY_KEY(Name)                                              \
    CHECK_RETURN_RESULT_ON_ERROR(_Visitor.Property(#Name, Name, true))

/// @brief Base class for all entities.
/// Derived classes must implement an IterateProperties function that takes a
/// visitor object and call its Property() function for each entity property.
/// You can use the ENTITY_PROPERTY and ENTITY_PROPERTY_KEY macros.
struct Entity
{
    INTEGER Id = -1; // primary key

    INTEGER GetId() const { return Id; }

    ResultCode CheckValidForCreate() const
    {
        // cant't insert with id
        if (Id != -1) return ResultCode::InvalidArgument;
        return ResultCode::OK;
    }

    ResultCode CheckValidForDelete() const
    {
        // need valid id
        if (Id == -1) return ResultCode::InvalidArgument;
        return ResultCode::OK;
    }

    ResultCode CheckValidForUpdate() const
    {
        // need valid id
        if (Id == -1) return ResultCode::InvalidArgument;
        return ResultCode::OK;
    }

    ResultCode CheckValues() const { return ResultCode::OK; }
};

/// @brief Store properties from a statement into an entity.
template <class TValue> ResultCode LoadEntity(TValue& _Entity, IStmt* _Stmt)
{
    Visitors::LoadFromStatementPropertyVisitor visitor{_Stmt};
    return _Entity.IterateProperties(visitor);
}

/// @brief Store entity properties into a statement.
template <class TEntity>
static ExpectedStmt Store(StmtPtr const& _Stmt, TEntity& _Entity)
{
    Visitors::StoreToStatementPropertyVisitor visitor{_Stmt};
    return {_Stmt, _Entity.IterateProperties(visitor)};
}

/// @brief Get a vector will all the ids of a collection of entities.
template <class TEntity>
ExpectedVector<DB::INTEGER> CollectIds(Vector<TEntity> const& _Entities)
{
    Vector<DB::INTEGER> Ids;
    Ids.resize(_Entities.size());
    std::ranges::transform(_Entities, std::begin(Ids), &TEntity::GetId);
    return Ids;
}

/// @brief Create a new entity in the database using values from the provided
/// entity.
template <class TEntity> Expected<TEntity> Create(DBPtr _DB, TEntity& _Entity)
{
    CHECK_RETURN_RESULT_ON_ERROR(_Entity.CheckValidForCreate());

    return DB::Query::InsertEntity<TEntity>(TEntity::Table, _Entity)
        .Prepare(_DB)
        .Then(&Store<TEntity>, _Entity)
        .Then(&IStmt::Step, true)
        .Then(
            [&](auto s)
            {
                auto entity = _Entity;
                entity.Id   = _DB->GetLastRowId();
                return Expected(entity, ResultCode::CreatedOK);
            });
}

/// @brief Try to retrieve a single entity from the database matching the given
/// key in the given column. Only one entity will be returned. If no entity is
/// found, an error will be returned.
template <class TEntity, class TValue>
static Expected<TEntity> GetWithKey(DBPtr _DB, StringView const& _KeyColumn,
                                    TValue const& _KeyValue)
{
    return DB::Query::Select(TEntity::Table)
        .Key(_KeyColumn)
        .Prepare(_DB)
        .Then(IStmt::BindValueFn<TValue>(), _KeyColumn, _KeyValue)
        .Then(&IStmt::ExecuteRow<TEntity>, true);
}

/// @brief Retrieve all entities of a given type from the database. If no
/// entities are found, an empty list will be returned. On error an error code
/// is returned.
template <class TEntity> ExpectedVector<TEntity> GetAll(DBPtr _DB)
{
    return DB::Query::Select(TEntity::Table)
        .Prepare(_DB)
        .Then(&IStmt::ExecuteList<TEntity>);
}

/// @brief Retrieve all entities of a given type from the database that match
/// the given key in the given column. If no entities are found, an empty list
/// will be returned. On error an error code is returned.
template <class TEntity, class TValue>
ExpectedVector<TEntity> GetAllWithKey(DBPtr _DB, StringView const& _KeyColumn,
                                      TValue const& _KeyValue)
{
    return DB::Query::Select(TEntity::Table)
        .Key(_KeyColumn)
        .Prepare(_DB)
        .Then(IStmt::BindValueFn<TValue>(), _KeyColumn, _KeyValue)
        .Then(&IStmt::ExecuteList<TEntity>);
}

/// @brief Update the values of an entity in the database. The entity must have
/// a valid ID set or an error code will be returned.
template <class TEntity> Expected<TEntity> Update(DBPtr _DB, TEntity& _Entity)
{
    CHECK_RETURN_RESULT_ON_ERROR(_Entity.CheckValidForUpdate());

    return DB::Query::UpdateEntity(TEntity::Table, _Entity)
        .Key("Id")
        .Prepare(_DB)
        .Then(&Store<TEntity>, _Entity)
        .Then(IStmt::BindValueFn<DB::INTEGER>(), "Id", _Entity.Id)
        .Then(&IStmt::Step, true)
        .ThenValue(_Entity);
}

/// @brief Delete an entity from the database. The entity must have a valid ID
/// set.
template <class TEntity> Expected<TEntity> Delete(DBPtr _DB, TEntity& _Entity)
{
    CHECK_RETURN_RESULT_ON_ERROR(_Entity.CheckValidForDelete());

    return DB::Query::Delete(TEntity::Table)
        .Key("Id")
        .Prepare(_DB)
        .Then(IStmt::BindValueFn<DB::INTEGER>(), "Id", _Entity.Id)
        .Then(&IStmt::Step, true)
        .Then(
            [&](auto s)
            {
                _Entity.Id = -1;
                return Expected{_Entity};
            });
}

} // namespace Booru::DB::Entities

namespace Booru
{
template <class TEntity>
concept CEntity =
    requires { requires std::is_base_of_v<DB::Entities::Entity, TEntity>; };
} // namespace Booru