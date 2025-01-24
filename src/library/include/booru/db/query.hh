#pragma once

#include "booru/db.hh"

namespace Booru::DB::Visitors
{
// Visitor that add all properties of an entity (except the primary key) to a query as a column. For
// update queries.
template <class Q>
class DBQueryNonPrimaryKeyColumnVisitor final
{
  public:
    DBQueryNonPrimaryKeyColumnVisitor( Q& _Query ) : Query{ _Query } {}

    template <class TValue>
    ResultCode Property( char const* _Name, TValue& _Value, bool _IsPrimaryKey = false )
    {
        if ( !_IsPrimaryKey )
        {
            Query.Column( _Name );
        }
        return ResultCode::OK;
    }

  private:
    Q& Query;
};

} // namespace Booru::DB::Visitors
namespace Booru::DB
{

// Abstract where condition for queries.
class QueryWhereCondition
{
  public:
    virtual ~QueryWhereCondition() {}
    virtual std::string ToString() = 0;
};

// Equality where condition
class QueryWhereEqualCondition : public QueryWhereCondition
{
  public:
    QueryWhereEqualCondition( char const* _A, char const* _B ) : A{ _A }, B{ _B } {}

    std::string ToString() override { return A + " == " + B; }

  protected:
    std::string A;
    std::string B;
};

// Abstract query base. Where conditions are ANDed together.
template <class Derived>
class Query
{
  public:
    Query( char const* _Table ) : Table{ _Table } {}

    virtual std::string ToString() = 0;

    // Tries to prepare the query into a statement.
    ExpectedOwning<DatabasePreparedStatementInterface> Prepare( DatabaseInterface& _DB )
    {
        std::string sqlString = ToString();
        return _DB.PrepareStatement( sqlString.c_str() );
    }

    // Add a column to the query.
    Derived& Column( std::string const& _Name )
    {
        Columns.push_back( _Name );
        return static_cast<Derived&>( *this );
    }

    // Add a where condition.
    Derived& WhereEqual( char const* _A, char const* _B )
    {
        WhereArgs.emplace_back( std::make_unique<QueryWhereEqualCondition>( _A, _B ) );
        return static_cast<Derived&>( *this );
    }

    // Add a key where condition (WHERE key = $key). Bind "$key" in the prepared statement.
    Derived& Key( char const* _Key )
    {
        std::string keyString = "$";
        keyString += _Key;
        return WhereEqual( _Key, keyString.c_str() );
    }

  protected:
    std::string Table;
    std::vector<std::string> Columns;
    std::vector<Owning<QueryWhereCondition>> WhereArgs;

    std::string GetWhereString()
    {
        if ( WhereArgs.empty() )
        {
            return "";
        }

        std::string whereString = "WHERE ";
        bool first              = true;
        for ( auto const& clause : WhereArgs )
        {
            if ( !first )
            {
                whereString += "AND ";
            }
            whereString += clause->ToString();
        }
        return whereString;
    }
};

// Simple select query. Queries given columns or all if none are specified.
class Select : public Query<Select>
{
  public:
    Select( char const* _Table ) : Query{ _Table } {}

    std::string ToString() override
    {
        std::string sqlString = "SELECT ";
        if ( Columns.empty() )
        {
            sqlString += "* \n";
        }
        else
        {
            bool first = true;
            for ( auto const& col : Columns )
            {
                if ( !first )
                {
                    sqlString += ", ";
                }
                sqlString += col;
                first = false;
            }
            sqlString += "\n";
        }
        if ( Table != "" )
        {
            sqlString += "FROM " + Table + "\n";
        }
        sqlString += GetWhereString();
        return sqlString;
    }
};

class Insert : public Query<Insert>
{
  public:
    Insert( char const* _Table ) : Query{ _Table } {}

    std::string ToString() override
    {
        std::string sqlString = "INSERT INTO " + Table + "\n(";

        bool first = true;
        for ( auto const& col : Columns )
        {
            if ( !first )
            {
                sqlString += ", ";
            }
            sqlString += col;
            first = false;
        }
        sqlString += ")\nVALUES(";
        first = true;
        for ( auto const& col : Columns )
        {
            if ( !first )
            {
                sqlString += ", ";
            }
            sqlString += "$";
            sqlString += col;
            first = false;
        }
        sqlString += ")\n";
        return sqlString;
    }
};

class Update : public Query<Update>
{
  public:
    Update( char const* _Table ) : Query{ _Table } {}

    std::string ToString() override
    {
        std::string sqlString = "UPDATE " + Table + "\nSET ";

        bool first = true;
        for ( auto const& col : Columns )
        {
            if ( !first )
            {
                sqlString += ", ";
            }
            sqlString += col + " = $" + col;
            first = false;
        }
        sqlString += " " + GetWhereString();
        return sqlString;
    }
};

class Delete : public Query<Delete>
{
  public:
    Delete( char const* _Table ) : Query{ _Table } {}

    std::string ToString() override
    {
        if ( WhereArgs.empty() )
        {
            // not doing a delete without a where
            // TODO: error!
            return "--";
        }

        std::string sqlString = "DELETE FROM " + Table + " " + GetWhereString();
        return sqlString;
    }
};

template <class TValue>
class InsertEntity : public Insert
{
  public:
    InsertEntity( char const* _Table, TValue& _Entity ) : Insert( _Table )
    {
        auto Visitor = Visitors::DBQueryNonPrimaryKeyColumnVisitor( *this );
        auto result  = _Entity.IterateProperties( Visitor );
    }
};

template <class TValue>
class UpdateEntity : public Update
{
  public:
    UpdateEntity( char const* _Table, TValue& _Entity ) : Update( _Table )
    {
        auto Visitor = Visitors::DBQueryNonPrimaryKeyColumnVisitor( *this );
        auto result  = _Entity.IterateProperties( Visitor );
    }
};

} // namespace Booru::DB
