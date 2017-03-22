set(PROG_PING "example_ping")
list(APPEND EXAMPLE_PING_SRCS "ping.cc")
add_executable(${PROG_PING} ${EXAMPLE_PING_SRCS})
target_link_libraries(${PROG_PING} "alpha")

set(PROG_PONG "example_pong")
list(APPEND EXAMPLE_PONG_SRCS "pong.cc")
add_executable(${PROG_PONG} ${EXAMPLE_PONG_SRCS})
target_link_libraries(${PROG_PONG} "alpha")
