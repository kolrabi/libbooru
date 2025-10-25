#pragma once

#include "booru/db.hh"

struct sqlite3;
class DatabasePreparedStatementInterface;

namespace Booru::DB::Sqlite3
{

    class DatabaseSqlite3 : public DB::DatabaseInterface
    {
    public:
        static ExpectedDB OpenDatabase(StringView const &_Path);

        explicit DatabaseSqlite3(sqlite3 *_Handle);
        virtual ~DatabaseSqlite3() override;

        virtual ExpectedStmt PrepareStatement(StringView const &_SQL) override;
        virtual ResultCode ExecuteSQL(StringView const &_SQL) override;

        virtual bool IsInTransaction() const override;
        virtual ResultCode BeginTransaction() override;
        virtual ResultCode CommitTransaction() override;
        virtual ResultCode RollbackTransaction() override;

        virtual Expected<DB::INTEGER> GetLastRowId() override;

    private:
        sqlite3 *m_Handle = nullptr;

        int TransactionDepth = 0;
        bool TransactionFailed = false;
    };

}
