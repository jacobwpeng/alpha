list(APPEND AMQP_CPP_SRCS
  "MethodArgTypes.cc"
  "DecodeUnit.cc"
  "CodedInputStream.cc"
  "Frame.cc"
  "FrameCodec.cc"
  "FieldValue.cc"
  "FieldTable.cc"
  "main.cc"
)
add_executable("AMQP-cpp" ${AMQP_CPP_SRCS})
target_link_libraries("AMQP-cpp" "alpha")
