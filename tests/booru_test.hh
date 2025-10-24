#pragma once

#include <cstdlib>

#include "entities.hh"
#include <booru/string.hh>

#define LOGGER "booru_test"

#define TEST_CHECK(cond) test_check((cond), #cond)
#define TEST_CHECK_ERROR(cond) test_check_error((cond), #cond)
#define TEST_CHECK_EQUAL(cond, v) { test_check((cond), #cond); test_equal((cond).Value, (v), #cond, #v); }

#define TEST_EQUAL(cond, v) { test_equal((cond), (v), #cond, #v); }
#define TEST_TRUE(cond) { test_equal(!!(cond), true, #cond); }
#define TEST_FALSE(cond) { test_equal(!(cond), true, #cond); }

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


static inline void test_check_error(Booru::ResultCode _Code, char const * _Cond)
{
    if (!Booru::ResultIsError(_Code))
    {
        LOG_ERROR("{} == {}\n", _Cond, Booru::ResultToString(_Code));
        FAIL();
    }
    LOG_INFO("{} == {}\n", _Cond, Booru::ResultToString(_Code));
}

template<class T, class U>
static inline void test_equal(T const & a, U const & b, char const * _CondA, char const * _CondB)
{
    auto aStr = Booru::ToString(a);
    auto bStr = Booru::ToString(b);
    if ( !(a == b) )
    {
        LOG_ERROR("{} != {}\nA = {}\nB = {}", _CondA, _CondB, aStr, bStr);
        FAIL();
    }
    LOG_INFO("{} == {}\nA = {}\nB = {}", _CondA, _CondB, aStr, bStr);
}

