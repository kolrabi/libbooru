#pragma once

#include <booru/db.hh>
#include <booru/db/stmt.hh>

namespace Booru::DB::Visitors
{

// Visitor that loads data from a statement into an entity property.
template <class TStmt> class LoadFromStatementPropertyVisitor
{
  public:
    LoadFromStatementPropertyVisitor(TStmt _Stmt) : m_Stmt{_Stmt} {}

    template <class TValue>
    ResultCode Property(StringView const& _Name, TValue& _Value,
                        bool _IsPrimaryKey = false)
    {
        return m_Stmt->GetColumnValue(_Name, _Value);
    }

  protected:
    TStmt m_Stmt;
};

// Visitor that binds data from an entity property to a statement.
class StoreToStatementPropertyVisitor
{
  public:
    StoreToStatementPropertyVisitor(StmtPtr const& _Stmt) : m_Stmt{_Stmt} {}

    template <class TValue>
    ResultCode Property(StringView const& _Name, TValue const& _Value,
                        bool _IsPrimaryKey = false)
    {
        return m_Stmt->BindValue(_Name, _Value);
    }

  protected:
    StmtPtr m_Stmt;
};

// Visitor that add all properties of an entity (except the primary key) to a
// query as a column. For update queries.
template <class TQuery> class QueryNonPrimaryKeyColumnVisitor final
{
  public:
    explicit QueryNonPrimaryKeyColumnVisitor(TQuery& _Query) : Query{_Query} {}

    template <class TValue>
    ResultCode Property(StringView const& _Name, TValue& _Value,
                        bool _IsPrimaryKey = false)
    {
        if (!_IsPrimaryKey) { Query.Column(_Name); }
        return ResultCode::OK;
    }

  private:
    TQuery& Query;
};

/// (Debug tool) Visitor that converts data into a printable string.
class ToStringPropertyVisitor
{
  public:
    explicit ToStringPropertyVisitor(StringView const& _Indent = "    ")
        : m_Indent{_Indent}
    {
    }

    String m_String;
    String m_Indent;

    template <class TValue>
    ResultCode Property(StringView const& _Name, TValue const& _Value,
                        bool _IsKey = false)
    {
        if (!m_String.empty()) { m_String += ", "; }
        m_String += String(_Name);
        m_String += "= '" + ToString(_Value) + "'";
        return ResultCode::OK;
    }
};

} // namespace Booru::DB::Visitors
