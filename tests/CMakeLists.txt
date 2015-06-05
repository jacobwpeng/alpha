cmake_minimum_required(VERSION 2.6)
set(EXE "gtest-all")

find_library(GTEST "gtest")
find_library(PTHREAD "pthread" )
list(APPEND SRC_LIST 
            gtest-all.cc
            unittest-skiplist.cc
    )

add_executable(${EXE} ${SRC_LIST})
target_link_libraries(${EXE} "alpha" ${GTEST})
add_test("gtest-all" ${EXE})
