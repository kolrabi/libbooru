#include <booru/result.hh>
#include <sqlite3.h>

namespace Booru
{

ResultCode Sqlite3ToResult(int _RC)
{
    switch (_RC)
    {
    case SQLITE_OK:
        return ResultCode::OK;
    case SQLITE_ROW:
        return ResultCode::DatabaseRow;
    case SQLITE_DONE:
        return ResultCode::DatabaseEnd;
    case SQLITE_ERROR:
        return ResultCode::DatabaseError;
    case SQLITE_RANGE:
        return ResultCode::DatabaseRangeError;

    case SQLITE_CONSTRAINT:
        return ResultCode::DatabaseConstraintViolation;
    case SQLITE_CONSTRAINT_FOREIGNKEY:
        return ResultCode::DatabaseFKeyViolation;
    case SQLITE_CONSTRAINT_PRIMARYKEY:
    case SQLITE_CONSTRAINT_ROWID:
        return ResultCode::DatabasePKeyViolation;
    case SQLITE_CONSTRAINT_UNIQUE:
        return ResultCode::AlreadyExists;
    case SQLITE_CONSTRAINT_NOTNULL:
        return ResultCode::DatabaseNotNullViolation;

    case SQLITE_BUSY:
        return ResultCode::DatabaseLocked;
    case SQLITE_LOCKED:
        return ResultCode::DatabaseTableLocked;
    }

    if (_RC > 256) { return Sqlite3ToResult(_RC & 0xFF); }

    return ResultCode::DatabaseError;
}

} // namespace Booru
