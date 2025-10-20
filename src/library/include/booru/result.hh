#pragma once

#include <booru/common.hh>
#include <booru/log.hh>

namespace Booru
{

enum class ResultCode : int32_t
{
    Undefined = std::numeric_limits<int32_t>::min(),

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
    DatabaseConstraintViolation = -2004
};

[[nodiscard]] char const* ResultToString( ResultCode _Result );
[[nodiscard]] char const* ResultToDescription( ResultCode _Result );

[[nodiscard]] static inline bool ResultIsError( ResultCode _Result )
{
    return static_cast<int32_t>( _Result ) < 0;
}

/// @brief Data structure representing an expected value and a possibly unexpected result/error code.
/// @tparam TValue Type of expected data value.
template <class TValue>
struct [[nodiscard]] Expected
{
    ResultCode Code;
    TValue Value;

    Expected() : Code { ResultCode::Undefined }, Value {} {}

    /// @brief C'tor that sets the result code and sets the value to default.
    /// @param _Code Code to set, by default OK.
    Expected( ResultCode _Code ) : Code{ _Code }, Value{} {}

    /// @brief Successful value constructor. Result code and value will be moved.
    /// @param _Value Value to set.
    /// @param _Code Result code to set, defaults to OK.
    Expected( TValue&& _Value, ResultCode _Code = ResultCode::OK ) : Code{ _Code }, Value{ std::move( _Value ) } {}

    /// @brief Returns true if result code is success, meaning Value is valid.
    operator bool() const { return !ResultIsError( Code ); }

    /// @brief Implicit operator to convert to ResultCode.
    operator ResultCode() const { return Code; }

    /// @brief Implicit operator to retrieve value.
    operator TValue() const { return Value; }

    /// @brief Builds an Expected object from a result code and a value object.
    /// _Object is only moved if _Code is a success code.
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
using ExpectedVector = Expected<std::vector<TValue>>;

// helper functions for result checking and logging 

template<class TValue, class...Args>
static inline bool CheckResult( String const & _LoggerName, TValue const & _Result, StringView const & _ResultStr, StringView const & _Func)
{
    auto logger = log4cxx::Logger::getLogger(_LoggerName);
    if ( ResultIsError(_Result) )
    {
        LOG4CXX_ERROR_FMT( logger, "{}(): {} failed: {}", _Func, _ResultStr, ResultToString(_Result) );
        return true;
    }
    LOG4CXX_TRACE_FMT( logger, "{}(): {} OK ({})!", _Func, _ResultStr, ResultToString(_Result) );
    return false;
}

template<class TValue, class...TArgs>
static inline bool CheckResult( String const & _LoggerName, TValue const & _Result, StringView const & _ResultStr, StringView const & _Func, StringView const & _Msg, TArgs const & ...args)
{
    auto logger = log4cxx::Logger::getLogger(_LoggerName);
    if ( ResultIsError(_Result) )
    {
        LOG4CXX_ERROR_FMT( logger, "{}(): {} failed: {}", _Func, _ResultStr, ResultToString(_Result) );
        LOG4CXX_ERROR_FMT( logger, "{}", std::vformat(_Msg, std::make_format_args(args...)) );
        return true;
    }
    LOG4CXX_TRACE_FMT( logger, "{}(): {} OK ({})!", _Func, _ResultStr, ResultToString(_Result) );
    LOG4CXX_TRACE_FMT( logger, "{}", std::vformat(_Msg, std::make_format_args(args...)) );
    return false;
}

// helper macros

// Check x, On error: log error with var args, resume.
#define CHECK( x, ... )\
    { auto const & _result = (x); CheckResult(LOGGER, _result, #x, __func__ __VA_OPT__(,) __VA_ARGS__); }

// Check x. On error: log error with var args, return error code.
#define CHECK_RETURN_RESULT_ON_ERROR( x, ... )\
    { auto const & _result = (x); if (CheckResult(LOGGER, _result, #x, __func__ __VA_OPT__(,) __VA_ARGS__)) return (ResultCode)_result; }

// Check x. On error: log error with var args, continue in loop.
#define CHECK_CONTINUE_ON_ERROR( x, ... )\
    { auto const & _result = (x); if (CheckResult(LOGGER, _result, #x, __func__ __VA_OPT__(,) __VA_ARGS__)) continue; }

// Check x. On error: log error with var args, break from loop.
#define CHECK_BREAK_ON_ERROR( x, ... )\
    { auto const & _result = (x); if (CheckResult(LOGGER, _result, #x, __func__ __VA_OPT__(,) __VA_ARGS__)) break; }

// Assert that x is true, logs failure.
#define CHECK_ASSERT( x )\
    { LOG4CXX_ASSERT_FMT( log4cxx::Logger::getLogger(LOGGER), x, "Assertion failed: {}", #x); }

}
