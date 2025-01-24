#include "stmt.hh"

#include "db.hh"
#include "result.hh"

#include <sqlite3.h>

namespace Booru::DB::Sqlite3
{

DatabasePreparedStatementSqlite3::DatabasePreparedStatementSqlite3( sqlite3_stmt* _Handle )
    : m_Handle{ _Handle }
{
    CHECK_CONDITION_RETURN( _Handle != nullptr );
}

DatabasePreparedStatementSqlite3::~DatabasePreparedStatementSqlite3()
{
    if ( m_Handle )
    {
        sqlite3_finalize( m_Handle );
        m_Handle = nullptr;
    }
}

ResultCode DatabasePreparedStatementSqlite3::GetParamIndex( char const* _Name, INTEGER& _Index )
{
    CHECK_CONDITION_RETURN_ERROR( m_Handle != nullptr );
    CHECK_CONDITION_RETURN_ERROR( _Name != nullptr );

    if ( _Name[0] != '$' )
    {
        std::string NameString( "$" );
        NameString += _Name;
        return GetParamIndex( NameString.c_str(), _Index );
    }

    _Index = sqlite3_bind_parameter_index( m_Handle, _Name );
    return ResultCode::OK;
}

ResultCode DatabasePreparedStatementSqlite3::BindValue( char const* _Name, void const* _Blob,
                                                        size_t _Size )
{
    if ( !_Blob )
    {
        return BindNull( _Name );
    }

    INTEGER paramIndex;
    CHECK_RESULT_RETURN_ERROR( GetParamIndex( _Name, paramIndex ) );
    if ( paramIndex == 0 )
    {
        return ResultCode::OK; // OK, not all queries use all entity members
    }

    // make a copy
    void* blobCopy = ::malloc( _Size );
    assert( blobCopy );
    ::memcpy( blobCopy, _Blob, _Size );

    // give it to sqlite, let it call free after use
    return Sqlite3ToResult( sqlite3_bind_blob( m_Handle, paramIndex, blobCopy, _Size, ::free ) );
}

ResultCode DatabasePreparedStatementSqlite3::BindValue( char const* _Name, FLOAT _Value )
{
    INTEGER paramIndex;
    CHECK_RESULT_RETURN_ERROR( GetParamIndex( _Name, paramIndex ) );
    if ( paramIndex == 0 )
    {
        return ResultCode::OK; // OK, not all queries use all entity members
    }
    return Sqlite3ToResult( sqlite3_bind_double( m_Handle, paramIndex, _Value ) );
}

ResultCode DatabasePreparedStatementSqlite3::BindValue( char const* _Name, INTEGER _Value )
{
    INTEGER paramIndex;
    CHECK_RESULT_RETURN_ERROR( GetParamIndex( _Name, paramIndex ) );
    if ( paramIndex == 0 )
    {
        return ResultCode::OK; // OK, not all queries use all entity members
    }
    return Sqlite3ToResult( sqlite3_bind_int64( m_Handle, paramIndex, _Value ) );
}

ResultCode DatabasePreparedStatementSqlite3::BindValue( char const* _Name, TEXT const& _Value )
{
    INTEGER paramIndex;
    CHECK_RESULT_RETURN_ERROR( GetParamIndex( _Name, paramIndex ) );
    if ( paramIndex == 0 )
    {
        return ResultCode::OK; // OK, not all queries use all entity members
    }

    // create a copy
    char* stringCopy = ::strdup( _Value.c_str() );
    CHECK_CONDITION_RETURN_ERROR( stringCopy != nullptr );

    //fprintf( stderr, "%p: Bind value '%s' to '%s' (%ld)\n", this, stringCopy, _Name, paramIndex );

    // give it to sqlite, let it free() it when it's done
    return Sqlite3ToResult( sqlite3_bind_text( m_Handle, paramIndex, stringCopy, -1, ::free ) );
}

ResultCode DatabasePreparedStatementSqlite3::BindNull( char const* _Name )
{
    INTEGER paramIndex;
    CHECK_RESULT_RETURN_ERROR( GetParamIndex( _Name, paramIndex ) );
    if ( paramIndex == 0 )
    {
        return ResultCode::OK; // OK, not all queries use all entity members
    }
    return Sqlite3ToResult( sqlite3_bind_null( m_Handle, paramIndex ) );
}

ResultCode DatabasePreparedStatementSqlite3::GetColumnValue( int _Index, void const*& _Blob,
                                                             size_t& _Size )
{
    if ( ColumnIsNull( _Index ) )
    {
        return ResultCode::ValueIsNull;
    }

    _Blob = sqlite3_column_blob( m_Handle, _Index );
    _Size = sqlite3_column_bytes( m_Handle, _Index );
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
    CHECK_CONDITION_RETURN_ERROR( str != nullptr );
    _Value = str;
    return ResultCode::OK;
}

bool DatabasePreparedStatementSqlite3::ColumnIsNull( int _Index )
{
    return sqlite3_column_type( m_Handle, _Index ) == SQLITE_NULL;
}

Expected<int> DatabasePreparedStatementSqlite3::GetColumnIndex( char const* _Name )
{
    int columnCount = sqlite3_column_count( m_Handle );
    for ( int i = 0; i < columnCount; i++ )
    {
        char const* name = sqlite3_column_name( m_Handle, i );
        if ( ::strcasecmp( name, _Name ) == 0 )
        {
            return i;
        }
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

ExpectedOwning<DatabaseInterface> DatabaseSqlite3::OpenDatabase( char const* _Path )
{
    sqlite3* db_handle = nullptr;
    int sqlite_result  = sqlite3_open( _Path, &db_handle );
    if ( sqlite_result == SQLITE_OK )
    {
        return { std::make_unique<DatabaseSqlite3>( db_handle ) };
    }
    return Sqlite3ToResult( sqlite_result );
}

} // namespace Booru::DB::Sqlite3
