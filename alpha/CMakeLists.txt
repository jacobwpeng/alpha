file(GLOB SRCS "*.cc")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DALPHA_STRIP_LOG=2")
include_directories(${PROJECT_SOURCE_DIR})
add_library("alpha" STATIC ${SRCS})
