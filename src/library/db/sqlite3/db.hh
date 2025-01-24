#pragma once

#include "booru/db.hh"

struct sqlite3;
class DatabasePreparedStatementInterface;

namespace Booru::DB::Sqlite3
{

class DatabaseSqlite3 : public DB::DatabaseInterface
{
  public:
    static ExpectedOwning<DatabaseInterface> OpenDatabase( char const* _Path );

    DatabaseSqlite3( sqlite3* _Handle );
    virtual ~DatabaseSqlite3();

    virtual ExpectedOwning<DatabasePreparedStatementInterface>
    PrepareStatement( char const* _SQL ) override;
    virtual ResultCode ExecuteSQL( char const* _SQL ) override;

    virtual ResultCode BeginTransaction() override;
    virtual ResultCode CommitTransaction() override;
    virtual ResultCode RollbackTransaction() override;

    virtual ResultCode GetLastRowId( INTEGER& _Id ) override;

  private:
    sqlite3* m_Handle = nullptr;

    int TransactionDepth   = 0;
    bool TransactionFailed = false;
};

}
