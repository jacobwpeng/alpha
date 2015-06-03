cmake_minimum_required(VERSION 2.6)

project(alpha)

include_directories(${PROJECT_SOURCE_DIR})
set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -Werror -D_FILE_OFFSET_BITS=64")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter -Wno-unused-private-field")
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # using Clang
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-const-variable")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # using gcc
    set(CMAKE_EXE_LINKER_FLAGS "-static-libstdc++")
endif()

add_subdirectory(alpha)
add_subdirectory(examples)# EXCLUDE_FROM_ALL)
