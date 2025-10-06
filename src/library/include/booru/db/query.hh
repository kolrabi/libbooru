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
    ResultCode Property( StringView const & _Name, TValue& _Value, bool _IsPrimaryKey = false )
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
    virtual String ToString() = 0;
};

// Equality where condition
class QueryWhereEqualCondition : public QueryWhereCondition
{
  public:
    QueryWhereEqualCondition( StringView const & _A, StringView const & _B ) : A{ _A }, B{ _B } {}

    String ToString() override { return A + " == " + B; }

  protected:
    String A;
    String B;
};

// Abstract query base. Where conditions are ANDed together.
template <class Derived>
class Query
{
  public:
    static inline const String LOGGER = "booru.db.query";

    Query( StringView const & _Table ) : Table{ _Table } {}

    virtual std::string ToString() = 0;

    // Tries to prepare the query into a statement.
    ExpectedOwning<DatabasePreparedStatementInterface> Prepare( DatabaseInterface& _DB )
    {
        String sqlString = ToString();
        return _DB.PrepareStatement( sqlString.c_str() );
    }

    // Add a column to the query.
    Derived& Column( StringView const & _Name )
    {
        Columns.push_back( String( _Name ) );
        return static_cast<Derived&>( *this );
    }

    // Add a where condition.
    Derived& WhereEqual( StringView const & _A, StringView const & _B )
    {
        WhereArgs.emplace_back( MakeOwning<QueryWhereEqualCondition>( _A, _B ) );
        return static_cast<Derived&>( *this );
    }

    // Add a key where condition (WHERE key = $key). Bind "$key" in the prepared statement.
    Derived& Key( StringView const & _Key )
    {
        String keyString = "$";
        keyString += _Key;
        return WhereEqual( _Key, keyString );
    }

  protected:
    String Table;
    StringVector Columns;
    Vector<Owning<QueryWhereCondition>> WhereArgs;

    String GetWhereString()
    {
        if ( WhereArgs.empty() )
        {
            return "";
        }

        String whereString = "WHERE ";
        bool first              = true;
        // TODO: whereString += Strings::Join( WhereArgs, " AND ");
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
    Select( StringView const & _Table ) : Query{ _Table } {}

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
    Insert( StringView const & _Table ) : Query{ _Table } {}

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
    Update( StringView const & _Table ) : Query{ _Table } {}

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
    Delete( StringView const & _Table ) : Query{ _Table } {}

    String ToString() override
    {
        if ( WhereArgs.empty() )
        {
            LOG_ERROR("Refusing to create a DELETE statement without a where clause!");
            // not doing a delete without a where
            return "--";
        }

        String sqlString = "DELETE FROM " + Table + " " + GetWhereString();
        return sqlString;
    }
};

template <class TValue>
class InsertEntity : public Insert
{
  public:
    InsertEntity( StringView const & _Table, TValue& _Entity ) : Insert( _Table )
    {
        auto Visitor = Visitors::DBQueryNonPrimaryKeyColumnVisitor( *this );
        auto result  = _Entity.IterateProperties( Visitor );
    }
};

template <class TValue>
class UpdateEntity : public Update
{
  public:
    UpdateEntity( StringView const & _Table, TValue& _Entity ) : Update( _Table )
    {
        auto Visitor = Visitors::DBQueryNonPrimaryKeyColumnVisitor( *this );
        auto result  = _Entity.IterateProperties( Visitor );
    }
};

} // namespace Booru::DB
