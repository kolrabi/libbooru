#include <booru/booru.hh>
#include <booru/db/entities/tag.hh>

#include <log4cxx/basicconfigurator.h>

#include <unistd.h>

#include "booru_test.hh"

#define TEST_CASE(name)                                                        \
    {                                                                          \
        #name, [](Booru::Booru& booru, const Booru::StringView& _Path) {
#define TEST_END                                                               \
    }                                                                          \
    }                                                                          \
    ,

static Booru::Vector<
    std::pair<Booru::String, void (*)(Booru::Booru&, const Booru::StringView&)>>
    test_cases = {TEST_CASE(open)
                  // try to delete it first
                  unlink(Booru::String(_Path).c_str());

// opening invalid pathshould result in error
TEST_CHECK_ERROR(booru.OpenDatabase("///", false));

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

int64_t schemaVersion = Booru::Booru::GetSchemaVersion();

TEST_CHECK_EQUAL(booru.GetConfig("db.version"), Booru::ToString(schemaVersion));
TEST_CHECK_EQUAL(booru.GetConfigInt64("db.version"), schemaVersion);

TEST_CHECK(booru.SetConfig("test.key", "test.value"));
TEST_CHECK_EQUAL(booru.GetConfig("test.key"), "test.value");
TEST_END

TEST_CASE(tag_create)
TEST_CHECK(booru.OpenDatabase(_Path, false));

Booru::DB::Entities::Tag tag;
TEST_EQUAL(tag.Id, -1);

tag.Name        = "test.tag";
tag.Description = "Test Tag";
tag.Rating      = Booru::DB::Entities::RATING_GENERAL;
tag.RedirectId.reset();

// should not exist -> error
tag.TagTypeId = 999;
TEST_CHECK_ERROR(booru.Create(tag));

// should exist (character) -> success
tag.TagTypeId = 3;
TEST_CHECK(booru.Create(tag).Update(tag));
TEST_EQUAL(tag.Id, 1);

// retrieve back from database, should have the same values
auto getResult = booru.GetTag("test.tag");
TEST_CHECK_EQUAL(getResult, tag);
TEST_END

TEST_CASE(tag_update)
TEST_CHECK(booru.OpenDatabase(_Path, false));

auto result = booru.GetTag("test.tag");
TEST_CHECK(result);

auto tag        = result.Value;

tag.Description = "Updated Tag";

// should not exist -> error
tag.TagTypeId   = 999;
TEST_CHECK_ERROR(booru.Update(tag));

// should exist (character) -> success
tag.TagTypeId = 4;
TEST_CHECK(booru.Update(tag));

// invalid id -> error
tag.Id = -1;
TEST_CHECK_ERROR(booru.Update(tag));
TEST_END

TEST_CASE(tag_retrieve)
TEST_CHECK(booru.OpenDatabase(_Path, false));

auto db = booru.GetDatabase();
TEST_CHECK(db);

auto result = booru.GetTag("test.tag");
TEST_CHECK(result);

auto tag          = result.Value;

auto resultVector = booru.GetTags();
TEST_CHECK(resultVector);
TEST_EQUAL(resultVector.Value.size(), 1);
TEST_EQUAL(resultVector.Value[0], tag);

auto idVector = resultVector.Then(
    Booru::DB::Entities::CollectIds<Booru::DB::Entities::Tag>);
TEST_CHECK(idVector);
TEST_EQUAL(idVector.Value.size(), 1);
TEST_EQUAL(idVector.Value[0], tag.Id);

// retrieve back by name, should have the same values
TEST_CHECK_EQUAL(booru.GetTag("test.tag"), tag);

// retrieve back by id
TEST_CHECK_EQUAL(booru.GetTag(1), tag);

resultVector = Booru::DB::Entities::GetAllWithKey<Booru::DB::Entities::Tag>(
    db, "Name", Booru::DB::TEXT("test.tag"));
TEST_CHECK(resultVector);
TEST_EQUAL(resultVector.Value.size(), 1);
TEST_EQUAL(resultVector.Value[0], tag);
TEST_END

TEST_CASE(tag_delete)
TEST_CHECK(booru.OpenDatabase(_Path, false));

// get our test tag
auto result = booru.GetTag("test.tag");
TEST_CHECK(result);

// try to delete
auto tag = result.Value;
TEST_CHECK(booru.Delete(tag).Update(tag));
// deletion should remove id
TEST_EQUAL(tag.Id, -1);

tag.Id = 1;
TEST_CHECK_ERROR(booru.Delete(tag));
TEST_END
}
;

static bool runTests(Booru::StringView dbName, Booru::StringView testName)
{
    // we need a booru library to check
    auto booru = Booru::Booru::InitializeLibrary();
    if (!booru) FAIL();

    // here you can set the log level to debug for more information about failed
    // tests
    // log4cxx::Logger::getRootLogger()->setLevel(log4cxx::Level::getDebug());

    LOG_INFO("Starting tests for database: {} and test: {}", dbName, testName);
    LOG_INFO("Current directory: {}", get_current_dir_name());

    // find test and run it, or run all tests (for coverage), or fail
    bool testAll     = (testName == "all");
    size_t testCount = 0;

    LOG_INFO("Running tests...");
    for (auto& [name, func] : test_cases)
    {
        if (testAll || name == testName)
        {
            LOG_INFO("Running test: {}", name);
            func(*booru, dbName);
            testCount++;
        }
    }

    if (testCount == 0)
    {
        LOG_ERROR("Unknown test {}\n", testName);
        return false;
    }

    return true;
}
// syntax: booru_test <db> <test>
int main(int argc, char* argv[])
{
    // initialize logging in case we have to speak before booru initializes it
    log4cxx::BasicConfigurator::configure();

    // check arguments
    if (argc != 3)
    {
        LOG_ERROR("Usage: {} <db> <test>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (runTests(argv[1], argv[2])) PASS();
    FAIL();
}