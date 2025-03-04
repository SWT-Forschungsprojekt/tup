cmake_minimum_required(VERSION 3.10)
project(tup)

# --- GTFS-RT PROTOBUF ---
add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated/gtfs-rt/gtfs-realtime.pb.h
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated/gtfs-rt/gtfs-realtime.pb.cc
        COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:protoc>
        --cpp_out=${CMAKE_CURRENT_BINARY_DIR}/generated/gtfs-rt
        --proto_path=${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/gtfs-realtime.proto
        DEPENDS protoc
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/gtfs-realtime.proto
        COMMENT "Generating GTFS RT protocol buffers C++ code"
)

add_library(gtfs-rt ${CMAKE_CURRENT_BINARY_DIR}/generated/gtfs-rt/gtfs-realtime.pb.cc)
target_compile_features(gtfs-rt PUBLIC cxx_std_23)
target_include_directories(gtfs-rt SYSTEM PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/generated)
target_link_libraries(gtfs-rt protobuf::libprotobuf)

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
target_link_libraries(tup-backend boost-json conf boost fmt utl geo web-server nigiri tbb curl gtfs-rt)
target_include_directories(tup-backend PRIVATE exe/backend/include)

# include custom configuration from motis-project
include(cmake/buildcache.cmake)
include(cmake/pkg.cmake)
