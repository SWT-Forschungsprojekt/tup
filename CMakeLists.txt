cmake_minimum_required(VERSION 3.10)
project(tup)

# Add include directories for dependencies
include_directories(
        ${CMAKE_SOURCE_DIR}/deps/boost
        ${CMAKE_SOURCE_DIR}/deps/conf/include
        ${CMAKE_SOURCE_DIR}/deps/fmt/include
        ${CMAKE_SOURCE_DIR}/deps/utl/include
        ${CMAKE_SOURCE_DIR}/deps/net/include
        ${CMAKE_SOURCE_DIR}/deps/net/web_server/include
        ${CMAKE_SOURCE_DIR}/deps/geo/include
)

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

# --- GTFS-RT PROTOBUF ---
add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated/gtfsrt/gtfs-realtime.pb.h
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated/gtfsrt/gtfs-realtime.pb.cc
        COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:protoc>
        --cpp_out=${CMAKE_CURRENT_BINARY_DIR}/generated/gtfsrt
        --proto_path=${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/gtfs-realtime.proto
        DEPENDS protoc
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/gtfs-realtime.proto
        COMMENT "Generating GTFS RT protocol buffers C++ code"
)
add_library(gtfsrt ${CMAKE_CURRENT_BINARY_DIR}/generated/gtfsrt/gtfs-realtime.pb.cc)
target_compile_features(gtfsrt PUBLIC cxx_std_23)
target_include_directories(gtfsrt SYSTEM PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/generated)
target_link_libraries(gtfsrt protobuf::libprotobuf)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(nigiri-compile-options
            -Weverything
            -Wno-c++98-compat
            -Wno-c++98-compat-pedantic
            -Wno-newline-eof
            -Wno-missing-prototypes
            -Wno-padded
            -Wno-double-promotion
            -Wno-undef
            -Wno-undefined-reinterpret-cast
            -Wno-float-conversion
            -Wno-global-constructors
            -Wno-exit-time-destructors
            -Wno-switch-enum
            -Wno-c99-designator
            -Wno-zero-as-null-pointer-constant
            -Wno-missing-noreturn
            -Wno-undefined-func-template
            -Wno-unsafe-buffer-usage
            -Wno-c++20-compat
            -Wno-reserved-macro-identifier
            -Wno-documentation-unknown-command
            -Wno-duplicate-enum
            -Wno-switch-default
            -Werror)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    set(nigiri-compile-options -Wall -Wextra -Werror)
elseif (MSVC)
    set(nigiri-compile-options /WX)
else ()
    set(nigiri-compile-options
            -Wall
            -Wextra
            -Wno-maybe-uninitialized)
    if (NOT CMAKE_CROSSCOMPILING)
        set(nigiri-compile-options ${nigiri-compile-options} -Werror)
    endif ()
endif ()

# --- Backend ---
file(GLOB_RECURSE tup-backend-src exe/backend/*.cc)
add_executable(tup-backend ${tup-backend-src})
target_link_libraries(tup-backend boost-json conf boost fmt utl geo web-server gtfsrt)
target_include_directories(tup-backend PRIVATE exe/backend/include)

# include custom configuration from motis-project
include(cmake/buildcache.cmake)
include(cmake/pkg.cmake)

# just for example add some compiler flags
target_compile_options(tup-backend PUBLIC ${nigiri-compile-options})
