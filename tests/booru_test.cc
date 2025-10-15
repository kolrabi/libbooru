
#include <booru/booru.hh>
#include <booru/db/entities/tag.hh>

#include <log4cxx/basicconfigurator.h>

#include <unistd.h>

#include "booru_test.hh"

#define TEST_CHECK(cond) test_check((cond), #cond)

#define TEST_CHECK_EQUAL(cond, v) { test_check((cond), #cond); test_equal((cond).Value, (v), #cond, #v); }
#define TEST_EQUAL(cond, v) { test_equal((cond), (v), #cond, #v); }
#define TEST_TRUE(cond) { test_equal(!!(cond), true, #cond); }
#define TEST_FALSE(cond) { test_equal(!(cond), true, #cond); }

static inline void test_check_error(Booru::ResultCode _Code, char const * _Cond)
{
    if (!Booru::ResultIsError(_Code))
    {
        LOG_ERROR("Condition is not error: {}\n", _Cond);
        FAIL();
    }
    LOG_INFO("Condition is error, as expected: {}\n", _Cond);
}

#define TEST_CHECK_ERROR(cond) test_check_error((cond), #cond)
#define TEST_CASE(name) {#name, [](Booru::Booru &booru, const Booru::StringView &_Path){
#define TEST_END }},

static Booru::Vector<std::pair<Booru::String, void(*)(Booru::Booru &, const Booru::StringView &)>> test_cases =
{
    TEST_CASE(open)
        // try to delete it first
        unlink(Booru::String(_Path).c_str());

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

    TEST_CASE(tag_create)
        TEST_CHECK(booru.OpenDatabase(_Path, false));

        Booru::DB::Entities::Tag tag;
        TEST_EQUAL(tag.Id, -1);

        tag.Name = "test.tag";
        tag.Description = "Test Tag";
        tag.Rating = Booru::DB::Entities::RATING_GENERAL;
        tag.RedirectId.reset();

        // should not exist -> error
        tag.TagTypeId = 999;
        TEST_CHECK_ERROR(booru.CreateTag(tag));

        // should exist (character) -> success
        tag.TagTypeId = 3;
        TEST_CHECK(booru.CreateTag(tag));
        TEST_EQUAL(tag.Id, 1);

        // retrieve back from database, should have the same values
        auto result = booru.GetTag("test.tag");
        TEST_CHECK_EQUAL(result, tag);
    TEST_END

    TEST_CASE(tag_update)
        TEST_CHECK(booru.OpenDatabase(_Path, false));

        auto result = booru.GetTag("test.tag");
        TEST_CHECK(result);

        auto tag = result.Value;

        tag.Description = "Updated Tag";
 
        // should not exist -> error
        tag.TagTypeId = 999;
        TEST_CHECK_ERROR(booru.UpdateTag(tag));

        // should exist (character) -> success
        tag.TagTypeId = 4;
        TEST_CHECK(booru.UpdateTag(tag));

        // retrieve back by name, should have the same values
        TEST_CHECK_EQUAL(booru.GetTag("test.tag"), tag);

        // retrieve back by id
        TEST_CHECK_EQUAL(booru.GetTag(1), tag);
    TEST_END

    TEST_CASE(tag_delete)
        TEST_CHECK(booru.OpenDatabase(_Path, false));

        // get our test tag
        auto result = booru.GetTag("test.tag");
        TEST_CHECK(result);

        // try to delete
        auto tag = result.Value;
        TEST_CHECK(booru.DeleteTag(tag));

        // deletion should remove id
        TEST_EQUAL(tag.Id, -1);

        // try to delete again, should succesfully delete all tags with the given id, of which there is none.
        tag.Id = 1;
        TEST_CHECK(booru.DeleteTag(tag));
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

    Booru::String dbName = argv[1];
    Booru::String testName = argv[2];

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
            booru.reset();
            FAIL();
        }
    }
    booru.reset();
    PASS();
}