#pragma once

#include "booru/db/types.hh"
#include "booru/result.hh"

namespace Booru::DB
{

/// @brief Interface for a prepared statement.
class DatabasePreparedStatementInterface
{
  public:
    virtual ~DatabasePreparedStatementInterface() {}

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // Bind values to statement arguments.
    // ////////////////////////////////////////////////////////////////////////////////////////////

    virtual ResultCode BindValue( char const* _Name, void const* _Blob, size_t _Size ) = 0;
    virtual ResultCode BindValue( char const* _Name, FLOAT _Value )                    = 0;
    virtual ResultCode BindValue( char const* _Name, INTEGER _Value )                  = 0;
    virtual ResultCode BindValue( char const* _Name, TEXT const& _Value )              = 0;
    virtual ResultCode BindNull( char const* _Name )                                   = 0;

    template <size_t BlobSize>
    ResultCode BindValue( char const* _Name, BLOB<BlobSize> const& _Value );

    template <class TValue>
    ResultCode BindValue( char const* _Name, NULLABLE<TValue> const& _Value );

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // Retrieve returned values from an executed statement.
    // ////////////////////////////////////////////////////////////////////////////////////////////

    virtual Expected<int> GetColumnIndex( char const* _Name ) = 0;

    virtual ResultCode GetColumnValue( int _Index, void const*& _Blob, size_t& _Size ) = 0;
    virtual ResultCode GetColumnValue( int _Index, FLOAT& _Value )                     = 0;
    virtual ResultCode GetColumnValue( int _Index, INTEGER& _Value )                   = 0;
    virtual ResultCode GetColumnValue( int _Index, TEXT& _Value )                      = 0;
    virtual bool ColumnIsNull( int _Index )                                            = 0;

    template <size_t BlobSize>
    ResultCode GetColumnValue( int _Index, BLOB<BlobSize>& _Value );

    template <class TValue>
    ResultCode GetColumnValue( int _Index, NULLABLE<TValue>& _Value );

    template <class TValue>
    ResultCode GetColumnValue( char const* _Name, TValue& _Value );

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // Execution
    // ////////////////////////////////////////////////////////////////////////////////////////////

    /// @brief Execute statement, advance to next row
    /// @param _NeedRow If true, return an error if there is no row returned.
    /// @return Result code.
    virtual ResultCode Step( bool _NeedRow = false ) = 0;

    /// @brief Execute statement, return single value. First row, first column.
    /// @tparam TValue Type of value to return.
    /// @param _NeedRow If true, return an error if there is no row returned.
    /// @return The expected value or an error.
    template <class TValue>
    Expected<TValue> ExecuteScalar( bool _NeedRow = false );

    /// @brief Execute statement, return a single row, store into an entity.
    /// @tparam TEntity Type of entity to return.
    /// @param _NeedRow If true, return an error if there is no row returned.
    /// @return The expected row or an error.
    template <class TEntity>
    Expected<TEntity> ExecuteRow( bool _NeedRow = false );

    /// @brief Execute statement, return all rows as entities.
    /// @tparam TEntity Type of entity to return.
    /// @return The expected entities or an error.
    template <class TEntity>
    ExpectedList<TEntity> ExecuteList();
};

// ////////////////////////////////////////////////////////////////////////////////////////////
// Template implementations
// ////////////////////////////////////////////////////////////////////////////////////////////

// Bind values.

template <size_t BlobSize>
ResultCode DatabasePreparedStatementInterface::BindValue( char const* _Name, BLOB<BlobSize> const& _Value )
{
    return BindValue( _Name, _Value.data(), _Value.size() );
}

template <class TValue>
ResultCode DatabasePreparedStatementInterface::BindValue( char const* _Name,
                                                          NULLABLE<TValue> const& _Value )
{
    if ( _Value.has_value() )
    {
        return BindValue( _Name, _Value.value() );
    }
    else
    {
        return BindNull( _Name );
    }
}

// GetColumnValue

template <size_t BlobSize>
ResultCode DatabasePreparedStatementInterface::GetColumnValue( int _Index, BLOB<BlobSize>& _Value )
{
    // get blob pointer and size
    void const* blobPtr = nullptr;
    size_t blobSize     = 0;
    CHECK_RESULT_RETURN_ERROR( GetColumnValue( _Index, blobPtr, blobSize ) );

    _Value.fill( 0 );
    if ( blobSize > BlobSize )
    {
        // TODO: warning? error? data got truncated
        blobSize = BlobSize;
    }
    else
    {
        // TODO: warning? data too short, padded with 0s
    }
    ::memcpy( _Value.data(), blobPtr, blobSize );
    return ResultCode::OK;
}

template <class TValue>
ResultCode DatabasePreparedStatementInterface::GetColumnValue( int _Index, NULLABLE<TValue>& _Value )
{
    if ( ColumnIsNull( _Index ) )
    {
        _Value.reset();
        return ResultCode::OK;
    }

    TValue v;
    ResultCode result = GetColumnValue( _Index, v );
    _Value            = v;
    return result;
}

template <class TValue>
ResultCode DatabasePreparedStatementInterface::GetColumnValue( char const* _Name, TValue& _Value )
{
    auto indexResult = GetColumnIndex( _Name );
    CHECK_RESULT_RETURN_ERROR( indexResult.Code );

    int index = indexResult.Value;
    return GetColumnValue( index, _Value );
}

// Execution

template <class TValue>
Expected<TValue> DatabasePreparedStatementInterface::ExecuteScalar( bool _NeedRow )
{
    CHECK_RESULT_RETURN_ERROR( Step( _NeedRow ) );

    TValue value;
    CHECK_RESULT_RETURN_ERROR( GetColumnValue( 0, value ) );
    return value;
}

template <class TEntity>
Expected<TEntity> DatabasePreparedStatementInterface::ExecuteRow( bool _NeedRow )
{
    CHECK_RESULT_RETURN_ERROR( Step( _NeedRow ) );

    TEntity value;
    CHECK_RESULT_RETURN_ERROR( LoadEntity( value, *this ) );
    return value;
}

template <class TEntity>
ExpectedList<TEntity> DatabasePreparedStatementInterface::ExecuteList()
{
    std::vector<TEntity> values;

    auto stepResult = Step();
    CHECK_RESULT_RETURN_ERROR( stepResult );
    while ( stepResult != ResultCode::DatabaseEnd )
    {
        TEntity value;
        CHECK_RESULT_RETURN_ERROR( LoadEntity( value, *this ) );
        values.push_back( value );
        stepResult = Step();
        CHECK_RESULT_RETURN_ERROR( stepResult );
    }
    return values;
}

}
