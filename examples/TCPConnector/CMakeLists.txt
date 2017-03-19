set(PROG "example_tcp_connector")
list(APPEND SRCS "main.cc")
add_executable(${PROG} ${SRCS})
target_link_libraries(${PROG} "alpha")
