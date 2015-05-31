file(GLOB SRCS "*.cc")
include_directories(${PROJECT_SOURCE_DIR})
add_library("alpha" STATIC ${SRCS})
