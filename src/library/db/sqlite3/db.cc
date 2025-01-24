#include "db.hh"

#include "result.hh"
#include "stmt.hh"

#include <sqlite3.h>

namespace Booru::DB::Sqlite3
{

DatabaseSqlite3::DatabaseSqlite3( sqlite3* _Handle ) : m_Handle{ _Handle }
{
    assert( m_Handle != nullptr );
    
    ResultCode rc = ExecuteSQL("PRAGMA foreign_keys = ON;");
    assert(rc == ResultCode::OK);
}

DatabaseSqlite3::~DatabaseSqlite3()
{
    if ( m_Handle )
    {
        sqlite3_close( m_Handle );
        m_Handle = nullptr;
    }
}

ExpectedOwning<DatabasePreparedStatementInterface>
DatabaseSqlite3::PrepareStatement( char const* _SQL )
{
    assert( m_Handle );

    // fprintf(stderr, "%p: preparing statement:\n%s\n", this, _SQL);

    sqlite3_stmt* stmt_handle = nullptr;
    int sqlite_result         = sqlite3_prepare_v3( m_Handle, _SQL, -1, 0, &stmt_handle, nullptr );
    if ( sqlite_result == SQLITE_OK )
    {
        return { std::make_unique<DatabasePreparedStatementSqlite3>( stmt_handle ) };
    }

    return Sqlite3ToResult( sqlite_result );
}

ResultCode DatabaseSqlite3::ExecuteSQL( char const* _SQL )
{
    assert( m_Handle );
    return Sqlite3ToResult( sqlite3_exec( m_Handle, _SQL, nullptr, nullptr, nullptr ) );
}

ResultCode DatabaseSqlite3::BeginTransaction()
{
    if ( TransactionDepth == 0 )
    {
        TransactionDepth++;
        TransactionFailed = false;
        return ExecuteSQL( "BEGIN TRANSACTION;" );
    }
    TransactionDepth++;
    return ResultCode::OK;
}

ResultCode DatabaseSqlite3::CommitTransaction()
{
    TransactionDepth--;
    if ( TransactionDepth == 0 )
    {
        if ( TransactionFailed )
        {
            return ExecuteSQL( "ROLLBACK;" );
        }
        else
        {
            return ExecuteSQL( "COMMIT;" );
        }
    }
    return ResultCode::OK;
}

ResultCode DatabaseSqlite3::RollbackTransaction()
{
    TransactionDepth--;
    if ( TransactionDepth == 0 )
    {
        return ExecuteSQL( "ROLLBACK;" );
    }
    TransactionFailed = true;
    return ResultCode::OK;
}

ResultCode DatabaseSqlite3::GetLastRowId( INTEGER& _Id )
{
    auto stmt = PrepareStatement( "SELECT last_insert_rowid();" );
    CHECK_RESULT_RETURN_ERROR( stmt.Code );
    auto result = stmt.Value->ExecuteScalar<INTEGER>( true );
    _Id         = result.Value;
    CHECK_RESULT_RETURN_ERROR( result.Code );
    return ResultCode::CreatedOK;
}

} // namespace Booru::DB::Sqlite3
