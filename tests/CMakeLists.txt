add_executable( booru_test booru_test.cc )

set_target_properties( booru_test PROPERTIES CXX_STANDARD 20 )
get_target_property( BOORU_INCLUDES booru INCLUDE_DIRECTORIES )
target_link_libraries( booru_test PRIVATE booru )
target_include_directories( booru_test PRIVATE ${BOORU_INCLUDES} )

add_test( open              booru_test "test.db" "open" )
add_test( config            booru_test "test.db" "config" )

