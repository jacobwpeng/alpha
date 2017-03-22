cmake_minimum_required(VERSION 3.0)
project(alpha CXX)

include_directories(${PROJECT_SOURCE_DIR})
set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -Werror -ggdb -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_FILE_OFFSET_BITS=64")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

include(CheckCXXCompilerFlag)

check_cxx_compiler_flag(-fno-reorder-blocks FLAG_SUPPORT_NO_REORDER_BLOCKS)
if (FLAG_SUPPORT_NO_REORDER_BLOCKS)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-reorder-blocks")
endif()

option(STATIC_LINK "Static link all libs" OFF)
if (STATIC_LINK)
  message(STATUS "Static link all libs")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

option(ALPHA_LOG_LEVEL "Library internal logging level")

add_subdirectory(alpha)
option(BUILD_EXAMPLES "Build examples" OFF)
if (BUILD_EXAMPLES)
  add_subdirectory(examples)
else()
  add_subdirectory(examples EXCLUDE_FROM_ALL)
endif()

add_subdirectory(tests)
enable_testing()
add_test(NAME "gtest-all" COMMAND gtest-all)
