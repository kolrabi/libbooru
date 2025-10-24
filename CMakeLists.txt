cmake_minimum_required( VERSION 3.23 )
project( libbooru VERSION 0.1.0 LANGUAGES CXX )


set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

enable_testing( )
include( CTest )

include( CMakePackageConfigHelpers )

option( CODE_COVERAGE "Enable code coverage" OFF )
if ( CODE_COVERAGE )
    include( CodeCoverage )
    append_coverage_compiler_flags()
endif()


add_subdirectory( "src" )
add_subdirectory( "tests" )


# Packaging
set(CPACK_PACKAGE_VENDOR "Björn Paetzel")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Image tagging library")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
set(CPACK_SOURCE_IGNORE_FILES
    /.git
    /dist
    /.*build.*
    /\\\\.DS_Store
)

include(CPack)
