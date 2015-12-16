cmake_minimum_required(VERSION 3.0)
project(alpha CXX)

include_directories(${PROJECT_SOURCE_DIR})
set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -Werror -ggdb -D_FILE_OFFSET_BITS=64")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
add_compile_options(-Wno-unused-parameter)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")

include(CheckCXXCompilerFlag)

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
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++")
endif()

add_subdirectory(alpha)
option(BUILD_EXAMPLES "Build examples" OFF)
if (BUILD_EXAMPLES)
  add_subdirectory(examples)# EXCLUDE_FROM_ALL)
else()
  add_subdirectory(examples EXCLUDE_FROM_ALL)
endif()
