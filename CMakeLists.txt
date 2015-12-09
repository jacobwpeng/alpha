cmake_minimum_required(VERSION 3.0)
project(alpha CXX)

include_directories(${PROJECT_SOURCE_DIR})
add_compile_options(-std=c++11)
add_compile_options(-Wall -Wextra -Werror)
add_compile_options(-D_FILE_OFFSET_BITS=64)
add_compile_options("$<$<CONFIG:DEBUG>:-ggdb>")
add_compile_options("$<$<CONFIG:RELEASE>:-O3 -DNDEBUG>")
add_compile_options(-Wno-unused-parameter)

include(CheckCXXCompilerFlag)

check_cxx_compiler_flag(-Wunused-const-variable FLAG_SUPPORT_UNUSED_CONST_VARIABLE)
if(FLAG_SUPPORT_UNUSED_CONST_VARIABLE)
  add_compile_options(-Wno-unused-const-variable)
endif()

check_cxx_compiler_flag(-Wunused-private-field FLAG_SUPPORT_UNUSED_PRIVATE_FIELD)
if(FLAG_SUPPORT_UNUSED_PRIVATE_FIELD)
  add_compile_options(-Wno-unused-private-field)
endif()

option(STATIC_LINK_LIBSTDC++ "Static link to libstdc++" OFF)
if (STATIC_LINK_LIBSTDC++)
  set(CMAKE_EXE_LINKER_FLAGS "-static-libstdc++")
endif()

add_subdirectory(alpha)
add_subdirectory(examples)# EXCLUDE_FROM_ALL)
