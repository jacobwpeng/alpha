set(PROG "example_udp_server")
list(APPEND UDPSERVER_EXAMPLE_SRCS "main.cc")
add_executable(${PROG} ${UDPSERVER_EXAMPLE_SRCS})
target_link_libraries(${PROG} "alpha")
