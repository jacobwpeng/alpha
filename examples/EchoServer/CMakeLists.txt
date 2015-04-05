file(GLOB SRCS "*.cc")
list(APPEND ECHO_SERVER_EXAMPLE_SRCS "echo_server.cc")
add_executable("echo_server" ${ECHO_SERVER_EXAMPLE_SRCS})
target_link_libraries("echo_server" "alpha")
