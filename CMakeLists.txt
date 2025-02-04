cmake_minimum_required(VERSION 3.10)
project(tup)


# Libraries
file(GLOB_RECURSE tuplib-files src/*.cc)
add_library(tuplib ${tuplib-files})
target_include_directories(tuplib PUBLIC include)
target_link_libraries(tuplib nigiri rtree.c)

# Executable
file(GLOB_RECURSE tup-files exe/*.cc)
add_executable(tup ${tup-files})
target_compile_features(tup PUBLIC cxx_std_23)
target_compile_options(tup PRIVATE ${motis-compile-options})
target_link_libraries(tup PRIVATE
        tuplib
        conf
        nigiri
        utl
)

# include custom configuration from motis-project
include(cmake/buildcache.cmake)
include(cmake/pkg.cmake)

# just for example add some compiler flags
target_compile_options(tup PUBLIC)
