cmake_minimum_required( VERSION 3.22 )
project( libbooru VERSION 0.1.0 LANGUAGES CXX )
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)


enable_testing( )
include( CTest )


option( BUILD_STATIC "Build static library" OFF )

option( CODE_COVERAGE "Enable code coverage" OFF )
if ( CODE_COVERAGE )
    include( CodeCoverage )
    append_coverage_compiler_flags()
endif()


add_subdirectory( "src" )
add_subdirectory( "tests" )
