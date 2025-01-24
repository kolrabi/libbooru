#pragma once

#include "booru/db.hh"
#include "booru/db/stmt.hh"

namespace Booru::DB::Visitors
{

// Visitor that loads data from a statement into an entity property.
class LoadFromStatementPropertyVisitor
{
  public:
    LoadFromStatementPropertyVisitor( DB::DatabasePreparedStatementInterface& _Stmt ) : Stmt{ _Stmt } {}

    DB::DatabasePreparedStatementInterface& Stmt;

    template <class TValue>
    ResultCode Property( char const* _Name, TValue& _Value, bool _IsPrimaryKey = false )
    {
        return Stmt.GetColumnValue( _Name, _Value );
    }
};

// Visitor that binds data from an entity property to a statement.
class StoreToStatementPropertyVisitor
{
  public:
    StoreToStatementPropertyVisitor( DB::DatabasePreparedStatementInterface& _Stmt ) : Stmt{ _Stmt } {}

    DB::DatabasePreparedStatementInterface& Stmt;

    template <class TValue>
    ResultCode Property( char const* _Name, TValue& _Value, bool _IsPrimaryKey = false )
    {
        return Stmt.BindValue( _Name, _Value );
    }
};

// Visitor that converts data into a printable string.
class ToStringPropertyVisitor
{
  public:
    ToStringPropertyVisitor( String const & _Indent = "    " ) : m_Indent { _Indent } {}

    String m_String;
    String m_Indent;

    String NameString( String const & _Name ) const
    {
        String Name = _Name;
        Name += ":";
        if (Name.size() < 12)
        {
            Name.append(12-Name.size(), ' ');
        }
        return Name;
    }

    template <class TValue>
    ResultCode Property( char const* _Name, TValue& _Value, bool _IsPrimaryKey = false )
    {
        String Name = NameString(_Name);
        m_String += m_Indent;
        m_String += Name;
        m_String += ToString(_Value) + "\n";
        return ResultCode::OK;
    }
};

} // namespace Booru::DB::Visitors
