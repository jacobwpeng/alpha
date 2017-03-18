set(PROG "example_async_connection")
list(APPEND EXAMPLE_ASYNC_CONNECTION_SRCS "main.cc")
add_executable(${PROG} ${EXAMPLE_ASYNC_CONNECTION_SRCS})
target_link_libraries(${PROG} "alpha")
