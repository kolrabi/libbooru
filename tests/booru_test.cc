#include "booru_test.hh"

#include <booru/booru.hh>

#include <unistd.h>

using namespace Booru;

static constexpr String LOGGER = "test";

static inline void test_check(ResultCode _Code, char const * _Cond)
{
    if (Booru::ResultIsError(_Code))
    {
        LOG_ERROR("Condition is error: {}\n", _Cond);
        FAIL();
    }
}

#define TEST_CHECK(cond) test_check((cond), #cond)


template<class T, class U>
static inline void test_equal(T const & a, U const & b, char const * _Cond)
{
    if ( a != b )
    {
        LOG_ERROR("Not equal: {}: {} != {}\n", _Cond, a, b);
        FAIL();
    }
}

#define TEST_CHECK_EQUAL(cond, v) { test_check((cond), #cond); test_equal((cond).Value, (v), #cond); }

static inline void test_check_error(Booru::ResultCode _Code, char const * _Cond)
{
    if (!Booru::ResultIsError(_Code))
    {
        LOG_ERROR("Condition is not error: {}\n", _Cond);
        FAIL();
    }
}

#define TEST_CHECK_ERROR(cond) test_check_error((cond), #cond)

static void test_open(Booru::Booru &booru, const Booru::StringView &_Path)
{
    // try to delete it first
    unlink(String(_Path).c_str());

    // opening nonexisting database should result in error
    TEST_CHECK_ERROR(booru.OpenDatabase(_Path, false));

    // open it and create tables
    booru.CloseDatabase();
    TEST_CHECK(booru.OpenDatabase(_Path, true));

    // reopening should work now
    booru.CloseDatabase();
    TEST_CHECK(booru.OpenDatabase(_Path, false));
}

static void test_config(Booru::Booru &booru, const Booru::StringView &_Path)
{
   TEST_CHECK(booru.OpenDatabase(_Path, false));

   TEST_CHECK_EQUAL(booru.GetConfig("db.version"), "0"); // TODO: get current version from library
   TEST_CHECK_EQUAL(booru.GetConfigInt64("db.version"), 0); // TODO: get current version from library

   TEST_CHECK(booru.SetConfig("test.key", "test.value"));
   TEST_CHECK_EQUAL(booru.GetConfig("test.key"), "test.value");

}

#define TEST(n) if (testName == #n) test_##n(*booru, dbName); else

// syntax: booru_test <db> <test>
int main(int argc, char *argv[]) {
    if (argc != 3) {
        LOG_ERROR("Usage: {} <db> <test>\n", argv[0]);
        return EXIT_FAILURE;
    }
    auto booru = Booru::Booru::InitializeLibrary();
    if (!booru) 
        FAIL();

    Booru::StringView dbName = argv[1];
    Booru::StringView testName = argv[2];

    LOG_INFO("Starting tests for database: {} and test: {}", dbName, testName);
    LOG_INFO("Current directory: {}", get_current_dir_name());

    TEST(open)
    TEST(config)

    {
        LOG_ERROR("Unknown test {}\n", testName);
        FAIL();
    }
    PASS();
}