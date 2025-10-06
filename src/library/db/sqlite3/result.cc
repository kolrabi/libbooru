#include <booru/result.hh>
#include <sqlite3.h>

namespace Booru
{

ResultCode Sqlite3ToResult( int _RC )
{
    switch ( _RC )
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
        return ResultCode::AlreadyExists;
    case SQLITE_BUSY:
        return ResultCode::DatabaseLocked;
    case SQLITE_LOCKED:
        return ResultCode::DatabaseTableLocked;
    }
    return ResultCode::DatabaseError;
}

}
