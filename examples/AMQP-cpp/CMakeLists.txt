list(APPEND AMQP_CPP_SRCS
  "main.cc"
  "DecodeUnit.cc"
  "CodedInputStream.cc"
  "Frame.cc"
  "FrameCodec.cc")
add_executable("AMQP-cpp" ${AMQP_CPP_SRCS})
target_link_libraries("AMQP-cpp" "alpha")
