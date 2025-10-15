add_executable( booru_test booru_test.cc )

if (CODE_COVERAGE)
    setup_target_for_coverage_lcov(
        NAME        coverage 
        EXECUTABLE  booru_test "test.db" "all" 
        EXCLUDE     "/usr/include/*" "tests/booru_test.*"
    )
endif()

set_target_properties( booru_test PROPERTIES CXX_STANDARD 20 )
get_target_property( BOORU_INCLUDES booru INCLUDE_DIRECTORIES )
target_link_libraries( booru_test PRIVATE booru )
target_include_directories( booru_test PRIVATE ${BOORU_INCLUDES} )

add_test( open              booru_test "test.db" "open" )
add_test( config            booru_test "test.db" "config" )

add_test( tag_create        booru_test "test.db" "tag_create" )
add_test( tag_update        booru_test "test.db" "tag_update" )
add_test( tag_delete        booru_test "test.db" "tag_delete" )
