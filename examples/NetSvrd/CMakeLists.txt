set(PROG "netsvrd")
list(APPEND NETSVRD_SRCS
  "netsvrd_frame_codec.cc"
  "netsvrd_virtual_server.cc"
  "netsvrd_app.cc"
  "netsvrd_main.cc"
)

add_executable(${PROG} ${NETSVRD_SRCS})
target_link_libraries(${PROG} "alpha")
