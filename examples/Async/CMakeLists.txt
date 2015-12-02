set(PROG "async")
list(APPEND ASYNC_CPP_SRCS "main.cc")
add_executable(${PROG} ${ASYNC_CPP_SRCS})
target_link_libraries(${PROG} "alpha")
