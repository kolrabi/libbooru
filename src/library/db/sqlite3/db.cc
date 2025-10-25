#include <booru/result.hh>

#include "db.hh"

#include "result.hh"
#include "stmt.hh"
#include "booru/log.hh"

#include <sqlite3.h>

namespace Booru::DB::Sqlite3
{

    static inline const String LOGGER = "booru.db.sqlite3";

    ExpectedDB DatabaseSqlite3::OpenDatabase(StringView const &_Path)
    {
        sqlite3 *db_handle = nullptr;
        int sqlite_result = sqlite3_open(String(_Path).c_str(), &db_handle);

        if (sqlite_result == SQLITE_OK)
        {
            // enable foreign keys constraint enforcement
            sqlite_result = sqlite3_exec(db_handle, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
        }

        if (sqlite_result == SQLITE_OK)
        {
            return {MakeShared<DatabaseSqlite3>(db_handle)};
        }
        return Sqlite3ToResult(sqlite_result);
    }

    DatabaseSqlite3::DatabaseSqlite3(sqlite3 *_Handle) : m_Handle{_Handle}
    {
        CHECK_ASSERT(m_Handle != nullptr);
    }

    DatabaseSqlite3::~DatabaseSqlite3()
    {
        if (m_Handle)
        {
            LOG_INFO("Closing database handle");
            sqlite3_close(m_Handle);
            m_Handle = nullptr;
        }
    }

    ExpectedStmt
    DatabaseSqlite3::PrepareStatement(StringView const &_SQL)
    {
        CHECK_ASSERT(m_Handle != nullptr);

        sqlite3_stmt *stmt_handle = nullptr;
        int sqlite_result = sqlite3_prepare_v3(m_Handle, String(_SQL).c_str(), -1, 0, &stmt_handle, nullptr);
        if (sqlite_result == SQLITE_OK)
        {
            LOG_DEBUG("Prepared statement: SQL was:\n{}", _SQL);

            return {MakeShared<DatabasePreparedStatementSqlite3>(stmt_handle)};
        }

        LOG_ERROR("Prepare statement failed with sqlite3 error code {}, SQL was:\n{}", sqlite_result, _SQL);
        return Sqlite3ToResult(sqlite_result);
    }

    ResultCode DatabaseSqlite3::ExecuteSQL(StringView const &_SQL)
    {
        CHECK_ASSERT(m_Handle != nullptr);

        int sqlite_result = sqlite3_exec(m_Handle, String(_SQL).c_str(), nullptr, nullptr, nullptr);
        if (sqlite_result == SQLITE_OK)
        {
            return ResultCode::OK;
        }

        LOG_ERROR("Execute SQL failed with sqlite3 error code {}, SQL was:\n{}", sqlite_result, _SQL);
        return Sqlite3ToResult(sqlite_result);
    }

    bool DatabaseSqlite3::IsInTransaction() const
    {
        return TransactionDepth > 0;
    }

    ResultCode DatabaseSqlite3::BeginTransaction()
    {
        if (TransactionDepth == 0)
        {
            LOG_INFO("Start of transaction");
            TransactionDepth++;
            TransactionFailed = false;
            return ExecuteSQL("BEGIN TRANSACTION;");
        }

        LOG_DEBUG("Nesting transaction");
        TransactionDepth++;
        return ResultCode::OK;
    }

    ResultCode DatabaseSqlite3::CommitTransaction()
    {
        LOG_DEBUG("Decreasing transaction depth");
        TransactionDepth--;
        if (TransactionDepth == 0)
        {
            if (TransactionFailed)
            {
                LOG_INFO("End of transaction, rolling back...");
                return ExecuteSQL("ROLLBACK;");
            }
            else
            {
                LOG_INFO("End of transaction, committing...");
                return ExecuteSQL("COMMIT;");
            }
        }
        return ResultCode::OK;
    }

    ResultCode DatabaseSqlite3::RollbackTransaction()
    {
        TransactionDepth--;
        if (TransactionDepth == 0)
        {
            LOG_INFO("Rolling back transaction");
            return ExecuteSQL("ROLLBACK;");
        }
        LOG_INFO("Decreased transaction depth. Transaction will be rolled back.");
        TransactionFailed = true;
        return ResultCode::OK;
    }

    Expected<INTEGER> DatabaseSqlite3::GetLastRowId()
    {
        return PrepareStatement("SELECT last_insert_rowid();")
            .Then([](auto s)
                  { return s->template ExecuteScalar<INTEGER>(true); });
    }

} // namespace Booru::DB::Sqlite3
