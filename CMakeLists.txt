cmake_minimum_required(VERSION 2.6)

project(alpha)

set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -Werror -Wno-unused-parameter -D_FILE_OFFSET_BITS=64")
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # using Clang
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-const-variable")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # using gcc
    set(CMAKE_EXE_LINKER_FLAGS "-static-libstdc++")
endif()

include_directories(${PROJECT_SOURCE_DIR})

add_subdirectory(base)
add_subdirectory(bus)
add_subdirectory(logger)
add_subdirectory(logsvrd)
