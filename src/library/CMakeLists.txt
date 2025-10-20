find_package(SQLite3 REQUIRED)
find_package(log4cxx REQUIRED)

add_library(booru)
add_library(Booru::Booru ALIAS booru)

target_sources(
    booru

    PRIVATE
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

    PUBLIC 
        FILE_SET HEADERS
        BASE_DIRS include/

        FILES 
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
            include/booru/log.hh
            include/booru/result.hh
            include/booru/string.hh
            include/booru/types.hh

)

set_target_properties( 
    booru 
    PROPERTIES 
        CXX_STANDARD 20
        VERSION ${PROJECT_VERSION} 
        SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_link_libraries( 
    booru
    LINK_PUBLIC 
        SQLite::SQLite3 
        log4cxx 
)

write_basic_package_version_file(
    BooruConfigVersion.cmake
    VERSION 
        ${PACKAGE_VERSION}
    COMPATIBILITY 
        AnyNewerVersion
)

install(
    TARGETS 
        booru
    EXPORT 
        BooruTargets
    FILE_SET 
        HEADERS 
    DESTINATION 
        ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    EXPORT
        BooruTargets
    FILE
        BooruConfigVersion.cmake
    DESTINATION
        lib/cmake/booru
    NAMESPACE
        Booru::
)

