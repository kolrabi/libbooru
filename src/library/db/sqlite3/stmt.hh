#pragma once

#include "booru/db/stmt.hh"

struct sqlite3_stmt;

namespace Booru::DB::Sqlite3
{

class DatabasePreparedStatementSqlite3 : public DatabasePreparedStatementInterface
{
  public:
    DatabasePreparedStatementSqlite3( sqlite3_stmt* _Handle );
    virtual ~DatabasePreparedStatementSqlite3();

    ResultCode BindValue( StringView const & _Name, void const* _Blob, size_t _Size ) override;
    ResultCode BindValue( StringView const & _Name, FLOAT _Value ) override;
    ResultCode BindValue( StringView const & _Name, INTEGER _Value ) override;
    ResultCode BindValue( StringView const & _Name, TEXT const& _Value ) override;
    ResultCode BindNull(  StringView const & _Name ) override;

    ResultCode GetColumnValue( int _Index, void const*& _Blob, size_t& _Size ) override;
    ResultCode GetColumnValue( int _Index, FLOAT& _Value ) override;
    ResultCode GetColumnValue( int _Index, INTEGER& _Value ) override;
    ResultCode GetColumnValue( int _Index, TEXT& _Value ) override;
    bool ColumnIsNull( int _Index ) override;

    Expected<int> GetColumnIndex( StringView const & _Name ) override;
    ResultCode Step( bool _NeedRow = false ) override;

  private:
    sqlite3_stmt* m_Handle;

    ResultCode GetParamIndex( StringView const & _Name, INTEGER& _Index );
};

} // namespace Booru::DB::Sqlite3
