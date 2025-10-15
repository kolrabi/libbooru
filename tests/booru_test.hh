#pragma once

#include <cstdlib>

#include "entities.hh"
#include <booru/string.hh>

#define LOGGER "booru_test"

static inline void PASS()
{
    exit(EXIT_SUCCESS);
}

static inline void FAIL()
{
    exit(EXIT_FAILURE);
}

static inline void test_check(Booru::ResultCode _Code, char const * _Cond)
{
    if (Booru::ResultIsError(_Code))
    {
        LOG_ERROR("{} == {}\n", _Cond, Booru::ResultToString(_Code));
        FAIL();
    }
    LOG_INFO("{} == {}\n", _Cond, Booru::ResultToString(_Code));
}

template<class T, class U>
static inline void test_equal(T const & a, U const & b, char const * _CondA, char const * _CondB)
{
    auto aStr = Booru::Strings::From(a);
    auto bStr = Booru::Strings::From(b);
    if ( !(a == b) )
    {
        LOG_ERROR("{} != {}\nA = {}\nB = {}", _CondA, _CondB, aStr, bStr);
        FAIL();
    }
    LOG_INFO("{} == {}\nA = {}\nB = {}", _CondA, _CondB, aStr, bStr);
}

