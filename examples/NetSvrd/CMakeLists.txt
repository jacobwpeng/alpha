set(NET_SVRD "example_net_svrd")

list(APPEND EXAMPLE_NET_SVRD_SRCS
  "netsvrd_frame_codec.cc"
  "netsvrd_virtual_server.cc"
  "netsvrd_worker.cc"
  "netsvrd_app.cc"
  "netsvrd_main.cc"
)

add_executable(${NET_SVRD} ${EXAMPLE_NET_SVRD_SRCS})
target_link_libraries(${NET_SVRD} "alpha")

set(ECHO_WORKER "example_net_svrd_echo_worker")
list(APPEND EXAMPLE_NET_SVRD_ECHO_WORKER_SRCS "netsvrd_echo_worker.cc")
add_executable(${ECHO_WORKER} ${EXAMPLE_NET_SVRD_ECHO_WORKER_SRCS})
target_link_libraries(${ECHO_WORKER} "alpha")
