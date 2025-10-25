#pragma once

#include "booru/db/stmt.hh"

struct sqlite3_stmt;

namespace Booru::DB::Sqlite3
{

    class DatabasePreparedStatementSqlite3 : public DatabasePreparedStatementInterface
    {
    public:
        explicit DatabasePreparedStatementSqlite3(sqlite3_stmt *_Handle);
        virtual ~DatabasePreparedStatementSqlite3() override;

        ExpectedStmt BindValue(StringView const &_Name, ByteSpan const &_Blob) override;
        ExpectedStmt BindValue(StringView const &_Name, FLOAT _Value) override;
        ExpectedStmt BindValue(StringView const &_Name, INTEGER _Value) override;
        ExpectedStmt BindValue(StringView const &_Name, TEXT const &_Value) override;
        ExpectedStmt BindNull(StringView const &_Name) override;

        ExpectedStmt GetColumnValue(int _Index, ByteVector &) override;
        ExpectedStmt GetColumnValue(int _Index, FLOAT &_Value) override;
        ExpectedStmt GetColumnValue(int _Index, INTEGER &_Value) override;
        ExpectedStmt GetColumnValue(int _Index, TEXT &_Value) override;
        bool ColumnIsNull(int _Index) override;

        Expected<int> GetColumnIndex(StringView const &_Name) override;
        ExpectedStmt Step(bool _NeedRow = false) override;

    private:
        sqlite3_stmt *m_Handle;

        ResultCode GetParamIndex(StringView const &_Name, INTEGER &_Index);
    };

} // namespace Booru::DB::Sqlite3
