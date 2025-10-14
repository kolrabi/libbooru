#include "booru_test.hh"

#include <booru/booru.hh>

#include <log4cxx/basicconfigurator.h>

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
#define TEST_CASE(name) {#name, [](Booru::Booru &booru, const Booru::StringView &_Path){
#define TEST_END }},

static Vector<std::pair<String, void(*)(Booru::Booru &, const Booru::StringView &)>> test_cases =
{
    TEST_CASE(open)
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
    TEST_END

    TEST_CASE(config)
        TEST_CHECK(booru.OpenDatabase(_Path, false));

        TEST_CHECK_EQUAL(booru.GetConfig("db.version"), "0"); // TODO: get current version from library
        TEST_CHECK_EQUAL(booru.GetConfigInt64("db.version"), 0); // TODO: get current version from library

        TEST_CHECK(booru.SetConfig("test.key", "test.value"));
        TEST_CHECK_EQUAL(booru.GetConfig("test.key"), "test.value");
    TEST_END
};

// syntax: booru_test <db> <test>
int main(int argc, char *argv[]) {
    // initialize logging in case we have to speak before booru initializes it
    log4cxx::BasicConfigurator::configure();

    // check arguments
    if (argc != 3) {
        LOG_ERROR("Usage: {} <db> <test>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // we need a booru library to check
    auto booru = Booru::Booru::InitializeLibrary();
    if (!booru) 
        FAIL();

    // here you can set the log level to debug for more information about failed tests
    // log4cxx::Logger::getRootLogger()->setLevel(log4cxx::Level::getDebug());

    String dbName = argv[1];
    String testName = argv[2];

    LOG_INFO("Starting tests for database: {} and test: {}", dbName, testName);
    LOG_INFO("Current directory: {}", get_current_dir_name());

    // find test and run it, or run all tests (for coverage), or fail
    if (testName == "all")
    {
        LOG_INFO("Running all tests...");
        for (const auto& testCase : test_cases)
        {
            LOG_INFO("Running test: {}", testCase.first);
            testCase.second(*booru, dbName);
        }
    }
    else
    {
        auto it = test_cases.begin();
        while(it != test_cases.end())
        {
            if ( it->first == testName )
            {
                LOG_INFO("Running test: {}", it->first);
                it->second(*booru, dbName);
                break;
            }
            it++;
        }
        if (it == test_cases.end())
        {
            LOG_ERROR("Unknown test {}\n", testName);
            FAIL();
        }
    }
    PASS();
}