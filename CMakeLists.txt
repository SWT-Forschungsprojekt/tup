cmake_minimum_required(VERSION 3.10)
project(tup)

###############################################################################
## file globbing ##############################################################
###############################################################################

# these instructions search the directory tree when CMake is
# invoked and put all files that match the pattern in the variables
# `sources` and `data`
file(GLOB_RECURSE sources      exe/*.cc)
# you can use set(sources src/main.cpp) etc if you don't want to
# use globbing to find files automatically

###############################################################################
## target definitions #########################################################
###############################################################################

# add the data to the target, so it becomes visible in some IDE
add_executable(tup ${sources})

target_link_libraries(tup PRIVATE conf nigiri)

# include custom configuration from motis-project
include(cmake/buildcache.cmake)
include(cmake/pkg.cmake)

# just for example add some compiler flags
target_compile_options(tup PUBLIC)
