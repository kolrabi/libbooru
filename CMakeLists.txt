cmake_minimum_required( VERSION 3.22 )
project( libbooru VERSION 0.1.0 LANGUAGES CXX )
enable_testing( )

option( BUILD_STATIC "Build static library" OFF )

add_subdirectory( "src" )
