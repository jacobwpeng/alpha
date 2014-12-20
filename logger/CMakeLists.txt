cmake_minimum_required(VERSION 2.6)
set(CMAKE_CXX_COMPILER "clang++")

project(logger)

set(LIB "alpha-logger")
set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-const-variable")
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

list(APPEND srcs 
        logger_streambuf.cc 
        logger_stream.cc
        logger_formatter.cc
        logger.cc 
    )

add_library(${LIB} ${srcs})
add_executable("example" "example.cc")
target_link_libraries("example" ${LIB})
