#pragma once

#include "booru/common.hh"

#include <vector>

enum class ResultCode : int32_t
{
    OK          = 0,
    CreatedOK   = 1,
    DatabaseRow = 2,
    DatabaseEnd = 3,

    UnknownError     = -1,
    NotFound         = -2,
    NotImplemented   = -3,
    AlreadyExists    = -4,
    InvalidEntityId  = -5,
    InvalidArgument  = -6,
    ArgumentTooLong  = -7,
    ArgumentTooShort = -8,
    ValueIsNull      = -9,
    ConditionFailed  = -10,
    RecursionExceeded = -11,

    InvalidRequest = -1000,
    Unauthorized   = -1001,

    DatabaseError       = -2000,
    DatabaseLocked      = -2001,
    DatabaseTableLocked = -2002,
    DatabaseRangeError  = -2003,
};

[[nodiscard]] char const* ResultToString( ResultCode _Result );
[[nodiscard]] char const* ResultToDescription( ResultCode _Result );

[[nodiscard]] static inline bool ResultIsError( ResultCode _Result )
{
    return static_cast<int32_t>( _Result ) < 0;
}
template <class TValue>
struct [[nodiscard]] Expected
{
    using ValueType = TValue;

    ResultCode Code;
    TValue Value;

    Expected( ResultCode _Code = ResultCode::OK ) : Code{ _Code }, Value{} {}

    Expected( TValue&& _Value ) : Code{ ResultCode::OK }, Value{ std::move( _Value ) } {}

    operator bool() const { return !ResultIsError( Code ); }
    operator ResultCode() const { return Code; }
    operator TValue() const { return Value; }

    static inline Expected<TValue> ErrorOrObject( ResultCode _Code, TValue && _Object )
    {
        if (ResultIsError(_Code))
        {
            return _Code;
        }
        return _Object;
    }
};

template <class TValue>
using ExpectedOwning = Expected<Owning<TValue>>;
template <class TValue>
using ExpectedShared = Expected<Shared<TValue>>;
template <class TValue>
using ExpectedList = Expected<std::vector<TValue>>;

#define PRINT_RESULT( x, r )                                                                       \
    fprintf( stderr, "%s:%d: %s: ERROR: %s\n", __FILE__, __LINE__, x, ResultToString( r ) )

#define PRINT_MSG( FMT, ... )                                                                      \
    fprintf( stderr, "%s:%d: " FMT "\n", __FILE__, __LINE__ __VA_OPT__(, ) __VA_ARGS__ )

#define CHECK_RESULT_RETURN_ERROR_MSG( x, ... )                                                    \
    {                                                                                              \
        auto _result = ( x );                                                                      \
        if ( ResultIsError( _result ) )                                                            \
        {                                                                                          \
            PRINT_RESULT( #x, _result );                                                           \
            PRINT_MSG( __VA_ARGS__ );                                                              \
            return _result;                                                                        \
        }                                                                                          \
    }
#define CHECK_RESULT_CONTINUE_ERROR_MSG( x, ... )                                                  \
    {                                                                                              \
        auto _result = ( x );                                                                      \
        if ( ResultIsError( _result ) )                                                            \
        {                                                                                          \
            PRINT_RESULT( #x, _result );                                                           \
            PRINT_MSG( __VA_ARGS__ );                                                              \
            continue;                                                                              \
        }                                                                                          \
    }
#define CHECK_RESULT_BREAK_ERROR_MSG( x, ... )                                                     \
    {                                                                                              \
        auto _result = ( x );                                                                      \
        if ( ResultIsError( _result ) )                                                            \
        {                                                                                          \
            PRINT_RESULT( #x, _result );                                                           \
            PRINT_MSG( __VA_ARGS__ );                                                              \
            break;                                                                                 \
        }                                                                                          \
    }

#define CHECK_RESULT_RETURN_ERROR_PRINT( x )                                                       \
    {                                                                                              \
        auto _result = ( x );                                                                      \
        if ( ResultIsError( _result ) )                                                            \
        {                                                                                          \
            PRINT_RESULT( #x, _result );                                                           \
            return _result;                                                                        \
        }                                                                                          \
    }
#define CHECK_RESULT_CONTINUE_ERROR_PRINT( x )                                                     \
    {                                                                                              \
        auto _result = ( x );                                                                      \
        if ( ResultIsError( _result ) )                                                            \
        {                                                                                          \
            PRINT_RESULT( #x, _result );                                                           \
            continue;                                                                              \
        }                                                                                          \
    }
#define CHECK_RESULT_BREAK_ERROR_PRINT( x )                                                        \
    {                                                                                              \
        auto _result = ( x );                                                                      \
        if ( ResultIsError( _result ) )                                                            \
        {                                                                                          \
            PRINT_RESULT( #x, _result );                                                           \
            break;                                                                                 \
        }                                                                                          \
    }

#define CHECK_RESULT_RETURN_ERROR( x )                                                             \
    {                                                                                              \
        auto _result = ( x );                                                                      \
        if ( ResultIsError( _result ) )                                                            \
        {                                                                                          \
            return _result;                                                                        \
        }                                                                                          \
    }
#define CHECK_RESULT_CONTINUE_ERROR( x )                                                           \
    {                                                                                              \
        auto _result = ( x );                                                                      \
        if ( ResultIsError( _result ) )                                                            \
        {                                                                                          \
            continue;                                                                              \
        }                                                                                          \
    }
#define CHECK_RESULT_BREAK_ERROR( x )                                                              \
    {                                                                                              \
        auto _result = ( x );                                                                      \
        if ( ResultIsError( _result ) )                                                            \
        {                                                                                          \
            break;                                                                                 \
        }                                                                                          \
    }

#define CHECK_CONDITION_RETURN( x )                                                                \
    {                                                                                              \
        bool _result = ( x );                                                                      \
        if ( !_result )                                                                            \
        {                                                                                          \
            PRINT_RESULT( #x, ResultCode::ConditionFailed );                                       \
            return;                                                                                \
        }                                                                                          \
    }

#define CHECK_CONDITION_RETURN_ERROR( x )                                                          \
    {                                                                                              \
        bool _result = ( x );                                                                      \
        if ( !_result )                                                                            \
        {                                                                                          \
            PRINT_RESULT( #x, ResultCode::ConditionFailed );                                       \
            return ResultCode::ConditionFailed;                                                    \
        }                                                                                          \
    }
