file(GLOB SRCS "*.cc")
list(APPEND PING_PONG_CLIENT_EXAMPLE_SRCS "ping_pong_client.cc")
add_executable("ping_pong_client" ${PING_PONG_CLIENT_EXAMPLE_SRCS})
target_link_libraries("ping_pong_client" "alpha")

list(APPEND PING_PONG_SERVER_EXAMPLE_SRCS "ping_pong_server.cc")
add_executable("ping_pong_server" ${PING_PONG_SERVER_EXAMPLE_SRCS})
target_link_libraries("ping_pong_server" "alpha")
