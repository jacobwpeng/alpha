cmake_minimum_required(VERSION 3.0)
project(alpha CXX)

include_directories(${PROJECT_SOURCE_DIR})
set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -Werror -ggdb -fPIC -D_FILE_OFFSET_BITS=64")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--start-group")

include(CheckCXXCompilerFlag)

check_cxx_compiler_flag(-Og FLAG_SUPPORT_DEBUG_OPTIMIZE)
if (FLAG_SUPPORT_DEBUG_OPTIMIZE)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Og")
endif()

check_cxx_compiler_flag(-fno-reorder-blocks FLAG_SUPPORT_NO_REORDER_BLOCKS)
if (FLAG_SUPPORT_NO_REORDER_BLOCKS)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-reorder-blocks")
endif()

check_cxx_compiler_flag(-Wunused-parameter FLAG_SUPPORT_UNUSED_PARAMETER)
if(FLAG_SUPPORT_UNUSED_PARAMETER)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")
endif()

check_cxx_compiler_flag(-Wunused-const-variable FLAG_SUPPORT_UNUSED_CONST_VARIABLE)
if(FLAG_SUPPORT_UNUSED_CONST_VARIABLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-const-variable")
endif()

check_cxx_compiler_flag(-Wunused-private-field FLAG_SUPPORT_UNUSED_PRIVATE_FIELD)
if(FLAG_SUPPORT_UNUSED_PRIVATE_FIELD)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-private-field")
endif()

option(STATIC_LINK_LIBSTDC++ "Static link to libstdc++" OFF)
if (STATIC_LINK_LIBSTDC++)
  message(STATUS "Static link libstdc++")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++")
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
