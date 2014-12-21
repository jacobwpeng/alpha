cmake_minimum_required(VERSION 2.6)

project(bus)

set(LIB "alpha-bus")
set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-const-variable")
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

list(APPEND srcs 
        mmap_file.cc
        ring_buffer.cc
        process_bus.cc
    )

add_library(${LIB} ${srcs})
add_executable("example" example.cc)
target_link_libraries("example" ${LIB})
