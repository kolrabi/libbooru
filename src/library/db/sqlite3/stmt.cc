#include "stmt.hh"

#include "db.hh"
#include "result.hh"

#include <sqlite3.h>

#define LOGGER    "booru.db.sqlite3.stmt"

namespace Booru::DB::Sqlite3
{

DatabasePreparedStatementSqlite3::DatabasePreparedStatementSqlite3( sqlite3_stmt* _Handle )
    : m_Handle{ _Handle }
{
    CHECK_ASSERT( _Handle != nullptr );
}

DatabasePreparedStatementSqlite3::~DatabasePreparedStatementSqlite3()
{
    if ( m_Handle )
    {
        sqlite3_finalize( m_Handle );
        m_Handle = nullptr;
    }
}

ResultCode DatabasePreparedStatementSqlite3::GetParamIndex( StringView const & _Name, INTEGER& _Index )
{
    CHECK_ASSERT( m_Handle != nullptr );
    CHECK_ASSERT( !_Name.empty() );

    String NameString( "$" );
    NameString += _Name;

    _Index = sqlite3_bind_parameter_index( m_Handle, NameString.c_str() );
    return ResultCode::OK;
}

ResultCode DatabasePreparedStatementSqlite3::BindValue( StringView const & _Name, ByteSpan const &_Blob )
{
    INTEGER paramIndex;
    CHECK_RETURN_RESULT_ON_ERROR( GetParamIndex( _Name, paramIndex ) );
    if ( paramIndex == 0 )
    {
        return ResultCode::OK; // OK, not all queries use all entity members
    }

    // make a copy
    void* blobCopy = ::malloc( _Blob.size_bytes() );
    assert( blobCopy );
    ::memcpy( blobCopy, _Blob.data(), _Blob.size_bytes() );

    // give it to sqlite, let it call free after use
    return Sqlite3ToResult( sqlite3_bind_blob( m_Handle, paramIndex, blobCopy, _Blob.size_bytes(), ::free ) );
}

ResultCode DatabasePreparedStatementSqlite3::BindValue( StringView const & _Name, FLOAT _Value )
{
    INTEGER paramIndex;
    CHECK_RETURN_RESULT_ON_ERROR( GetParamIndex( _Name, paramIndex ) );
    if ( paramIndex == 0 )
    {
        return ResultCode::OK; // OK, not all queries use all entity members
    }
    return Sqlite3ToResult( sqlite3_bind_double( m_Handle, paramIndex, _Value ) );
}

ResultCode DatabasePreparedStatementSqlite3::BindValue( StringView const & _Name, INTEGER _Value )
{
    INTEGER paramIndex;
    CHECK_RETURN_RESULT_ON_ERROR( GetParamIndex( _Name, paramIndex ) );
    if ( paramIndex == 0 )
    {
        return ResultCode::OK; // OK, not all queries use all entity members
    }
    return Sqlite3ToResult( sqlite3_bind_int64( m_Handle, paramIndex, _Value ) );
}

ResultCode DatabasePreparedStatementSqlite3::BindValue( StringView const & _Name, TEXT const& _Value )
{
    INTEGER paramIndex;
    CHECK_RETURN_RESULT_ON_ERROR( GetParamIndex( _Name, paramIndex ) );
    if ( paramIndex == 0 )
    {
        return ResultCode::OK; // OK, not all queries use all entity members
    }

    // create a copy
    char* stringCopy = ::strdup( _Value.c_str() );
    CHECK_ASSERT( stringCopy != nullptr );

    // give it to sqlite, let it free() it when it's done
    return Sqlite3ToResult( sqlite3_bind_text( m_Handle, paramIndex, stringCopy, -1, ::free ) );
}

ResultCode DatabasePreparedStatementSqlite3::BindNull( StringView const & _Name )
{
    INTEGER paramIndex;
    CHECK_RETURN_RESULT_ON_ERROR( GetParamIndex( _Name, paramIndex ) );
    if ( paramIndex == 0 )
    {
        return ResultCode::OK; // OK, not all queries use all entity members
    }
    return Sqlite3ToResult( sqlite3_bind_null( m_Handle, paramIndex ) );
}

ResultCode DatabasePreparedStatementSqlite3::GetColumnValue( int _Index, ByteVector& _Blob )
{
    if ( ColumnIsNull( _Index ) )
    {
        return ResultCode::ValueIsNull;
    }

    _Blob.resize(sqlite3_column_bytes( m_Handle, _Index ));
    ::memcpy(_Blob.data(), sqlite3_column_blob( m_Handle, _Index ), _Blob.size());
    return ResultCode::OK;
}

ResultCode DatabasePreparedStatementSqlite3::GetColumnValue( int _Index, FLOAT& _Value )
{
    if ( ColumnIsNull( _Index ) )
    {
        return ResultCode::ValueIsNull;
    }

    _Value = sqlite3_column_double( m_Handle, _Index );
    return ResultCode::OK;
}

ResultCode DatabasePreparedStatementSqlite3::GetColumnValue( int _Index, INTEGER& _Value )
{
    if ( ColumnIsNull( _Index ) )
    {
        return ResultCode::ValueIsNull;
    }

    _Value = sqlite3_column_int64( m_Handle, _Index );
    return ResultCode::OK;
}

ResultCode DatabasePreparedStatementSqlite3::GetColumnValue( int _Index, TEXT& _Value )
{
    if ( ColumnIsNull( _Index ) )
    {
        return ResultCode::ValueIsNull;
    }

    char const* str = reinterpret_cast<char const*>( sqlite3_column_text( m_Handle, _Index ) );
    CHECK_ASSERT( str != nullptr );
    _Value = str;
    return ResultCode::OK;
}

bool DatabasePreparedStatementSqlite3::ColumnIsNull( int _Index )
{
    CHECK_ASSERT( m_Handle != nullptr );
    CHECK_ASSERT( _Index >= 0 );

    return sqlite3_column_type( m_Handle, _Index ) == SQLITE_NULL;
}

Expected<int> DatabasePreparedStatementSqlite3::GetColumnIndex( StringView const & _Name )
{
    int columnCount = sqlite3_column_count( m_Handle );
    for ( int i = 0; i < columnCount; i++ )
    {
        StringView name = sqlite3_column_name( m_Handle, i );
        if ( name == _Name )
            return i;
    }
    return ResultCode::NotFound;
}

ResultCode DatabasePreparedStatementSqlite3::Step( bool _NeedRow )
{
    assert( m_Handle );
    ResultCode resultCode = Sqlite3ToResult( sqlite3_step( m_Handle ) );
    if ( _NeedRow && resultCode == ResultCode::DatabaseEnd )
    {
        return ResultCode::NotFound;
    }
    return resultCode;
}

// ----------------------------------------------------------------
// DatabaseSqlite3

ExpectedOwning<DatabaseInterface> DatabaseSqlite3::OpenDatabase( StringView const & _Path )
{
    sqlite3* db_handle = nullptr;
    int sqlite_result  = sqlite3_open( String( _Path ).c_str(), &db_handle );
    if ( sqlite_result == SQLITE_OK )
    {
        return { MakeOwning<DatabaseSqlite3>( db_handle ) };
    }
    return Sqlite3ToResult( sqlite_result );
}

} // namespace Booru::DB::Sqlite3
