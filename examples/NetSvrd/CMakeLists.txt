set(PROG "netsvrd")
set(ECHO_WORKER "echo_worker")

list(APPEND NETSVRD_SRCS
  "netsvrd_frame_codec.cc"
  "netsvrd_virtual_server.cc"
  "netsvrd_worker.cc"
  "netsvrd_app.cc"
  "netsvrd_main.cc"
)
list(APPEND NETSVRD_ECHO_WORKER_SRCS "netsvrd_echo_worker.cc")

add_executable(${PROG} ${NETSVRD_SRCS})
target_link_libraries(${PROG} "alpha")

add_executable(${ECHO_WORKER} ${NETSVRD_ECHO_WORKER_SRCS})
target_link_libraries(${ECHO_WORKER} "alpha")
