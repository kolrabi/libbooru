#pragma once

#include "booru/db/stmt.hh"

struct sqlite3_stmt;

namespace Booru::DB::Sqlite3
{

class DatabasePreparedStatementSqlite3 : public DatabasePreparedStatementInterface
{
  public:
    explicit DatabasePreparedStatementSqlite3( sqlite3_stmt* _Handle );
    virtual ~DatabasePreparedStatementSqlite3() override;

    Expected<DatabasePreparedStatementInterface*> BindValue( StringView const & _Name,  ByteSpan const& _Blob ) override;
    // ResultCode BindValue( StringView const & _Name, void const* _Blob, size_t _Size ) override;
    Expected<DatabasePreparedStatementInterface*> BindValue( StringView const & _Name, FLOAT _Value ) override;
    Expected<DatabasePreparedStatementInterface*> BindValue( StringView const & _Name, INTEGER _Value ) override;
    Expected<DatabasePreparedStatementInterface*> BindValue( StringView const & _Name, TEXT const& _Value ) override;
    Expected<DatabasePreparedStatementInterface*> BindNull(  StringView const & _Name ) override;

    Expected<DatabasePreparedStatementInterface*> GetColumnValue( int _Index, ByteVector& ) override;
    Expected<DatabasePreparedStatementInterface*> GetColumnValue( int _Index, FLOAT& _Value ) override;
    Expected<DatabasePreparedStatementInterface*> GetColumnValue( int _Index, INTEGER& _Value ) override;
    Expected<DatabasePreparedStatementInterface*> GetColumnValue( int _Index, TEXT& _Value ) override;
    bool ColumnIsNull( int _Index ) override;

    Expected<int> GetColumnIndex( StringView const & _Name ) override;
    Expected<DatabasePreparedStatementInterface*> Step( bool _NeedRow = false ) override;

  private:
    sqlite3_stmt* m_Handle;

    ResultCode GetParamIndex( StringView const & _Name, INTEGER& _Index );
};

} // namespace Booru::DB::Sqlite3
