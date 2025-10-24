#pragma once

#include <booru/db.hh>
#include <booru/db/stmt.hh>

namespace Booru::DB::Visitors
{

// Statement property visitor base class
class StatementPropertyVisitor
{
public:
    explicit StatementPropertyVisitor( DB::DatabasePreparedStatementInterface* _Stmt ) : Stmt{ _Stmt } {}

protected:
    DB::DatabasePreparedStatementInterface* Stmt;
};

// Visitor that loads data from a statement into an entity property.
class LoadFromStatementPropertyVisitor : private StatementPropertyVisitor
{
  public:
    using StatementPropertyVisitor::StatementPropertyVisitor;

    template <class TValue>
    ResultCode Property( StringView const & _Name, TValue& _Value, bool _IsPrimaryKey = false )
    {
        return Stmt->GetColumnValue( _Name, _Value );
    }
};

// Visitor that binds data from an entity property to a statement.
class StoreToStatementPropertyVisitor : private StatementPropertyVisitor
{
  public:
    using StatementPropertyVisitor::StatementPropertyVisitor;

    template <class TValue>
    ResultCode Property( StringView const & _Name, TValue& _Value, bool _IsPrimaryKey = false )
    {
        return Stmt->BindValue( _Name, _Value );
    }
};

// Visitor that add all properties of an entity (except the primary key) to a query as a column. For
// update queries.
template <class TQuery>
class QueryNonPrimaryKeyColumnVisitor final
{
  public:
    explicit QueryNonPrimaryKeyColumnVisitor( TQuery& _Query ) : Query { _Query } { }

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
    TQuery& Query;
};

/// (Debug tool) Visitor that converts data into a printable string.
class ToStringPropertyVisitor
{
  public:
    explicit ToStringPropertyVisitor( StringView const & _Indent = "    " ) : m_Indent { _Indent } {}

    String m_String;
    String m_Indent;

    String NameString( StringView const & _Name ) const
    {
        String Name(_Name);
        Name += ":";
        if (Name.size() < 12)
        {
            Name.append(12-Name.size(), ' ');
        }
        return Name;
    }

    template <class TValue>
    ResultCode Property( StringView const & _Name, TValue& _Value, bool _IsKey = false )
    {
        m_String = Strings::Join(
            {
                m_String,
                m_Indent,
                NameString(_Name),
                ToString(_Value),
                "\n"s
            }, ""sv);
        return ResultCode::OK;
    }
};

} // namespace Booru::DB::Visitors
