#include <booru/result.hh>

namespace Booru {

char const* ResultToString( ResultCode _Code )
{
    switch ( _Code )
    {
    case ResultCode::OK:
        return "OK";
    case ResultCode::CreatedOK:
        return "OK, Entity Created";
    case ResultCode::DatabaseRow:
        return "OK, Next Row";
    case ResultCode::DatabaseEnd:
        return "OK, Finished";

    case ResultCode::UnknownError:
        return "Unknown Error";
    case ResultCode::NotFound:
        return "Object not Found";
    case ResultCode::NotImplemented:
        return "Not Implemented";
    case ResultCode::AlreadyExists:
        return "Already Exists";
    case ResultCode::InvalidEntityId:
        return "Invalid Entity Id";
    case ResultCode::InvalidArgument:
        return "Invalid Argument";
    case ResultCode::ArgumentTooLong:
        return "Argument Too Long";
    case ResultCode::ArgumentTooShort:
        return "Argument Too Short";
    case ResultCode::ValueIsNull:
        return "Value is NULL";
    case ResultCode::ConditionFailed:
        return "Condition Failed";
    case ResultCode::RecursionExceeded:
        return "Recursion Exceeded";

    case ResultCode::InvalidRequest:
        return "Invalid Request";
    case ResultCode::Unauthorized:
        return "Unauthorized";

    case ResultCode::DatabaseError:
        return "Database Error";
    case ResultCode::DatabaseLocked:
        return "Database Locked";
    case ResultCode::DatabaseTableLocked:
        return "Database Table Locked";
    case ResultCode::DatabaseRangeError:
        return "Database Range Error";
    case ResultCode::DatabaseConstraintViolation:
        return "Database Constraint Violation";

    default:
        return "Unknown Result Code";
    }
}

char const* ResultToDescription( ResultCode _Code )
{
    switch ( _Code )
    {
    case ResultCode::OK:
        return "The requested operation completed successfully";
    case ResultCode::CreatedOK:
        return "OK, the entity was created successfully";
    case ResultCode::DatabaseRow:
        return "OK, a new database row is available for reading";
    case ResultCode::DatabaseEnd:
        return "OK, no new database rows are available";
    case ResultCode::UnknownError:
        return "The requested operation could not be completed for unknown "
               "reasons";
    case ResultCode::NotFound:
        return "The requested object could not be found";
    case ResultCode::NotImplemented:
        return "The requested operation has not been implemented yet";
    case ResultCode::AlreadyExists:
        return "Could not create object because it already exists";
    case ResultCode::InvalidEntityId:
        return "Update or Delete was requested with an invalid entity Id";
    case ResultCode::InvalidArgument:
        return "A provided argument was invalid or malformed";
    case ResultCode::ArgumentTooLong:
        return "A provided argument was longer than expected";
    case ResultCode::ArgumentTooShort:
        return "A provided argument was shorter than expected";
    case ResultCode::InvalidRequest:
        return "Request could not be understood";
    case ResultCode::Unauthorized:
        return "Request could not be handled due to lack of permissions";
    case ResultCode::ValueIsNull:
        return "Tried to retrieve a value from database, but it was NULL.";
    case ResultCode::ConditionFailed:
        return "A pre or post condition has failed.";
    case ResultCode::RecursionExceeded:
        return "Recursion depth exceeded";

    case ResultCode::DatabaseError:
        return "The database reported an error";
    case ResultCode::DatabaseLocked:
        return "Could not access database, it is locked by another process";
    case ResultCode::DatabaseTableLocked:
        return "Request could not be handled due to a locked table";
    case ResultCode::DatabaseRangeError:
        return "Database library function parameter was out of range";
    case ResultCode::DatabaseConstraintViolation:
        return "Statement would violate a constraint. Either a primary key already exists or a required foreign key doesn't.";

    default:
        return "Unknown Result Code";
    }
}

}
