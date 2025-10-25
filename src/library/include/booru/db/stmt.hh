#pragma once

#include <booru/db/types.hh>
#include <booru/result.hh>

namespace Booru::DB
{

    /// @brief Interface for a prepared statement.
    class DatabasePreparedStatementInterface : public std::enable_shared_from_this<DatabasePreparedStatementInterface>
    {
        static inline const String LOGGER = "booru.db.stmt";

    public:
        virtual ~DatabasePreparedStatementInterface() {}

        // ////////////////////////////////////////////////////////////////////////////////////////////
        // Bind values to statement arguments.
        // ////////////////////////////////////////////////////////////////////////////////////////////

        virtual ExpectedStmt BindValue(StringView const &_Name, ByteSpan const &_Blob) = 0;
        virtual ExpectedStmt BindValue(StringView const &_Name, FLOAT _Value) = 0;
        virtual ExpectedStmt BindValue(StringView const &_Name, INTEGER _Value) = 0;
        virtual ExpectedStmt BindValue(StringView const &_Name, TEXT const &_Value) = 0;
        virtual ExpectedStmt BindNull(StringView const &_Name) = 0;

        template <size_t BlobSize>
        ExpectedStmt BindValue(StringView const &_Name, BLOB<BlobSize> const &_Value);

        template <class TValue>
        ExpectedStmt BindValue(StringView const &_Name, NULLABLE<TValue> const &_Value);

        // ////////////////////////////////////////////////////////////////////////////////////////////
        // Retrieve returned values from an executed statement.
        // ////////////////////////////////////////////////////////////////////////////////////////////

        virtual Expected<int> GetColumnIndex(StringView const &_Name) = 0;

        virtual ExpectedStmt GetColumnValue(int _Index, ByteVector &) = 0;
        virtual ExpectedStmt GetColumnValue(int _Index, FLOAT &_Value) = 0;
        virtual ExpectedStmt GetColumnValue(int _Index, INTEGER &_Value) = 0;
        virtual ExpectedStmt GetColumnValue(int _Index, TEXT &_Value) = 0;
        virtual bool ColumnIsNull(int _Index) = 0;

        template <size_t BlobSize>
        ResultCode GetColumnValue(int _Index, BLOB<BlobSize> &_Value);

        template <class TValue>
        ResultCode GetColumnValue(int _Index, NULLABLE<TValue> &_Value);

        template <class TValue>
        ResultCode GetColumnValue(StringView const &_Name, TValue &_Value);

        // ////////////////////////////////////////////////////////////////////////////////////////////
        // Execution
        // ////////////////////////////////////////////////////////////////////////////////////////////

        /// @brief Execute statement, advance to next row
        /// @param _NeedRow If true, return an error if there is no row returned.
        /// @return Result code.
        virtual ExpectedStmt Step(bool _NeedRow = false) = 0;

        /// @brief Execute statement, return single value. First row, first column.
        /// @tparam TValue Type of value to return.
        /// @param _NeedRow If true, return an error if there is no row returned.
        /// @return The expected value or an error.
        template <class TValue>
        Expected<TValue> ExecuteScalar(bool _NeedRow = false);

        /// @brief Execute statement, return a single row, store into an entity.
        /// @tparam TEntity Type of entity to return.
        /// @param _NeedRow If true, return an error if there is no row returned.
        /// @return The expected row or an error.
        template <class TEntity>
        Expected<TEntity> ExecuteRow(bool _NeedRow = false);

        /// @brief Execute statement, return all rows as entities.
        /// @tparam TEntity Type of entity to return.
        /// @return The expected entities or an error.
        template <class TEntity>
        ExpectedVector<TEntity> ExecuteList();
    };

    // ////////////////////////////////////////////////////////////////////////////////////////////
    // Template implementations
    // ////////////////////////////////////////////////////////////////////////////////////////////

    // Bind values.

    template <size_t BlobSize>
    ExpectedStmt DatabasePreparedStatementInterface::BindValue(StringView const &_Name, BLOB<BlobSize> const &_Value)
    {
        return BindValue(_Name, ByteSpan(_Value));
    }

    template <class TValue>
    ExpectedStmt DatabasePreparedStatementInterface::BindValue(StringView const &_Name, NULLABLE<TValue> const &_Value)
    {
        if (_Value.has_value())
        {
            return BindValue(_Name, _Value.value());
        }
        else
        {
            return BindNull(_Name);
        }
    }

    // GetColumnValue

    template <size_t BlobSize>
    ResultCode DatabasePreparedStatementInterface::GetColumnValue(int _Index, BLOB<BlobSize> &_Value)
    {
        // get blob pointer and size
        ByteVector blob;
        CHECK_RETURN_RESULT_ON_ERROR(GetColumnValue(_Index, blob));

        _Value.fill(0);
        if (blob.size() > BlobSize)
        {
            LOG_WARNING("Data of size {} got truncated trying to store in blob of size {}", blob.size(), BlobSize);
            blob.resize(BlobSize);
        }
        else if (blob.size() < BlobSize)
        {
            LOG_WARNING("Data of size {} got padded with zeroes trying to store in blob of size {}", blob.size(), BlobSize);
        }
        std::copy(std::begin(blob), std::end(blob), std::begin(_Value));
        return ResultCode::OK;
    }

    template <class TValue>
    ResultCode DatabasePreparedStatementInterface::GetColumnValue(int _Index, NULLABLE<TValue> &_Value)
    {
        if (ColumnIsNull(_Index))
        {
            _Value.reset();
            return ResultCode::OK;
        }

        TValue v;
        ResultCode result = GetColumnValue(_Index, v);
        _Value = v;
        return result;
    }

    template <class TValue>
    ResultCode DatabasePreparedStatementInterface::GetColumnValue(StringView const &_Name, TValue &_Value)
    {
        CHECK_VAR_RETURN_RESULT_ON_ERROR(index, GetColumnIndex(_Name))
        return GetColumnValue(index.Value, _Value);
    }

    // Execution

    template <class TValue>
    Expected<TValue> DatabasePreparedStatementInterface::ExecuteScalar(bool _NeedRow)
    {
        TValue value;

        auto result =
            Step(_NeedRow)
                .Then([&](auto s)
                      { return s->GetColumnValue(0, value); });

        return Expected<TValue>::ErrorOrObject(result, std::move(value));
    }

    template <class TEntity>
    Expected<TEntity> DatabasePreparedStatementInterface::ExecuteRow(bool _NeedRow)
    {
        CHECK_RETURN_RESULT_ON_ERROR(Step(_NeedRow));

        TEntity value;
        CHECK_RETURN_RESULT_ON_ERROR(LoadEntity(value, this));
        return value;
    }

    template <class TEntity>
    ExpectedVector<TEntity> DatabasePreparedStatementInterface::ExecuteList()
    {
        Vector<TEntity> values;

        auto stepResult = Step();
        CHECK_RETURN_RESULT_ON_ERROR(stepResult);
        while (stepResult != ResultCode::DatabaseEnd)
        {
            TEntity value;
            CHECK_RETURN_RESULT_ON_ERROR(LoadEntity(value, this));
            values.push_back(value);
            stepResult = Step();
            CHECK_RETURN_RESULT_ON_ERROR(stepResult);
        }
        return values;
    }

}
