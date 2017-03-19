set(PROG "example_simple_http_server")
list(APPEND SRCS "main.cc")
add_executable(${PROG} ${SRCS})
target_link_libraries(${PROG} "alpha")
