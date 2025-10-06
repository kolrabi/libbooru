#pragma once

#include <booru/common.hh>
#include <booru/log.hh>

#include <vector>

namespace Booru
{

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

#define CHECK( x, ... )\
    { auto const & _result = (x); CheckResult(LOGGER, _result, #x, __func__ __VA_OPT__(,) __VA_ARGS__); }

#define CHECK_RETURN_RESULT_ON_ERROR( x, ... )\
    { auto const & _result = (x); if (CheckResult(LOGGER, _result, #x, __func__ __VA_OPT__(,) __VA_ARGS__)) return (ResultCode)_result; }

#define CHECK_CONTINUE_ON_ERROR( x, ... )\
    { auto const & _result = (x); if (CheckResult(LOGGER, _result, #x, __func__ __VA_OPT__(,) __VA_ARGS__)) continue; }

#define CHECK_BREAK_ON_ERROR( x, ... )\
    { auto const & _result = (x); if (CheckResult(LOGGER, _result, #x, __func__ __VA_OPT__(,) __VA_ARGS__)) break; }

#define CHECK_ASSERT( x )\
    { LOG4CXX_ASSERT_FMT( log4cxx::Logger::getLogger(LOGGER), x, "Assertion failed: {}", #x); }

}
