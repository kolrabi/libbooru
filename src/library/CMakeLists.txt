find_package(SQLite3 REQUIRED)

set( booru_public_headers
    include/booru/booru.hh
    include/booru/common.hh
    include/booru/db.hh
    include/booru/db/visitors.hh
    include/booru/db/entity.hh
    include/booru/db/entities/post_file.hh
    include/booru/db/entities/post_site_id.hh
    include/booru/db/entities/post_tag.hh
    include/booru/db/entities/post_type.hh
    include/booru/db/entities/post.hh
    include/booru/db/entities/site.hh
    include/booru/db/entities/tag_implication.hh
    include/booru/db/entities/tag_type.hh
    include/booru/db/entities/tag.hh
    include/booru/db/query.hh
    include/booru/db/stmt.hh
    include/booru/db/types.hh
    include/booru/result.hh
    include/booru/string.hh
)

set( booru_sources
    library.cc

    booru.cc
    result.cc
    string.cc

    db/sqlite3/db.hh
    db/sqlite3/db.cc
    db/sqlite3/result.hh
    db/sqlite3/result.cc
    db/sqlite3/stmt.hh
    db/sqlite3/stmt.cc
    db/sql.hh
    db/sql.cc

    ${booru_public_headers}
)

if(BUILD_STATIC)
    add_library(booru STATIC ${booru_sources})
else(BUILD_STATIC)
    add_library(booru SHARED ${booru_sources})
endif(BUILD_STATIC)

set_target_properties(booru PROPERTIES 
    CXX_STANDARD 20
    VERSION ${PROJECT_VERSION} 
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_include_directories( booru PRIVATE include )
target_include_directories( booru PRIVATE ${SQLite3_INCLUDE_DIRS} )
target_include_directories( booru PUBLIC include/booru )

target_link_libraries( booru ${SQLite3_LIBRARIES} )

install(TARGETS booru
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} 
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/booru DESTINATION include)