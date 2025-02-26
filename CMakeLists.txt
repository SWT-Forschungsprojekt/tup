cmake_minimum_required(VERSION 3.10)
project(tup)

###############################################################################
## file globbing ##############################################################
###############################################################################

# these instructions search the directory tree when CMake is
# invoked and put all files that match the pattern in the variables
# `sources` and `data`
file(GLOB_RECURSE sources      exe/backend/*.cc)
# you can use set(sources src/main.cpp) etc if you don't want to
# use globbing to find files automatically

###############################################################################
## target definitions #########################################################
###############################################################################

# add the data to the target, so it becomes visible in some IDE

file(GLOB_RECURSE tup-backend-src exe/backend/*.cc)
add_executable(tup-backend ${tup-backend-src})
target_link_libraries(tup-backend boost-json conf boost fmt utl geo web-server nigiri tbb curl)
target_include_directories(tup-backend PRIVATE exe/backend/include)

# include custom configuration from motis-project
include(cmake/buildcache.cmake)
include(cmake/pkg.cmake)
