#include <booru/log.hh>
#include <booru/result.hh>

#include "db.hh"
#include "result.hh"
#include "stmt.hh"

#include <sqlite3.h>

namespace Booru::DB::Sqlite3
{

constexpr auto LOGGER = "booru.db.sqlite3";

ExpectedDB Backend::OpenDatabase(StringView const& _Path)
{
    sqlite3* db_handle = nullptr;
    int sqlite_result  = sqlite3_open(String(_Path).c_str(), &db_handle);

    if (sqlite_result == SQLITE_OK)
    {
        // enable foreign keys constraint enforcement
        sqlite_result = sqlite3_exec(db_handle, "PRAGMA foreign_keys = ON;",
                                     nullptr, nullptr, nullptr);
    }

    if (sqlite_result == SQLITE_OK) { return {MakeShared<Backend>(db_handle)}; }
    return Sqlite3ToResult(sqlite_result);
}

Backend::Backend(sqlite3* _Handle) : m_Handle{_Handle}
{
    CHECK_ASSERT(m_Handle != nullptr);
}

Backend::~Backend()
{
    if (m_Handle)
    {
        LOG_INFO("Closing database handle");
        sqlite3_close(m_Handle);
        m_Handle = nullptr;
    }
}

ExpectedStmt Backend::PrepareStatement(StringView const& _SQL)
{
    CHECK_ASSERT(m_Handle != nullptr);

    sqlite3_stmt* stmt_handle = nullptr;
    int sqlite_result = sqlite3_prepare_v3(m_Handle, String(_SQL).c_str(), -1,
                                           0, &stmt_handle, nullptr);
    if (sqlite_result == SQLITE_OK)
    {
        LOG_DEBUG("Prepared statement: SQL was:\n{}", _SQL);

        return {MakeShared<DatabasePreparedStatementSqlite3>(stmt_handle)};
    }

    LOG_ERROR(
        "Prepare statement failed with sqlite3 error code {}, SQL was:\n{}",
        sqlite_result, _SQL);
    return Sqlite3ToResult(sqlite_result);
}

ResultCode Backend::ExecuteSQL(StringView const& _SQL)
{
    CHECK_ASSERT(m_Handle != nullptr);

    sqlite3_exec(m_Handle, String(_SQL).c_str(), nullptr, nullptr, nullptr);
    auto result = Sqlite3ToResult(sqlite3_extended_errcode(m_Handle));

    if (ResultIsError(result))
    {
        LOG_ERROR("Execute SQL failed with result '{}', SQL was:\n{}",
                  ResultToString(result), _SQL);
    }
    return result;
}

bool Backend::IsInTransaction() const { return m_TransactionDepth > 0; }

ResultCode Backend::BeginTransaction()
{
    if (m_TransactionDepth == 0)
    {
        LOG_DEBUG("Start of transaction");
        m_TransactionDepth++;
        m_TransactionFailed = false;
        return ExecuteSQL("BEGIN TRANSACTION;");
    }

    LOG_DEBUG("Nesting transaction");
    m_TransactionDepth++;
    return ResultCode::OK;
}

ResultCode Backend::CommitTransaction()
{
    LOG_DEBUG("Decreasing transaction depth");
    m_TransactionDepth--;
    if (m_TransactionDepth == 0)
    {
        if (m_TransactionFailed)
        {
            LOG_DEBUG("End of transaction, rolling back...");
            return ExecuteSQL("ROLLBACK;");
        }
        else
        {
            LOG_DEBUG("End of transaction, committing...");
            return ExecuteSQL("COMMIT;");
        }
    }
    return ResultCode::OK;
}

ResultCode Backend::RollbackTransaction()
{
    m_TransactionDepth--;
    if (m_TransactionDepth == 0)
    {
        LOG_INFO("Rolling back transaction");
        return ExecuteSQL("ROLLBACK;");
    }
    LOG_INFO("Decreased transaction depth. Transaction will be rolled back.");
    m_TransactionFailed = true;
    return ResultCode::OK;
}

Expected<INTEGER> Backend::GetLastRowId()
{
    return PrepareStatement("SELECT last_insert_rowid();")
        .Then(&IStmt::ExecuteScalar<INTEGER>, true);
}

} // namespace Booru::DB::Sqlite3
