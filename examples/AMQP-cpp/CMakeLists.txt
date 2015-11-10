list(APPEND AMQP_CPP_SRCS
  "MethodArgTypes.cc"
  "CodedWriter.cc"
  "CodedInputStream.cc"
  "CodedOutputStream.cc"
  "EncodeUnit.cc"
  "DecodeUnit.cc"
  "CodecEnv.cc"
  "Frame.cc"
  "FrameCodec.cc"
  "FieldValue.cc"
  "FieldTable.cc"
  "main.cc"
)
add_executable("AMQP-cpp" ${AMQP_CPP_SRCS})
target_link_libraries("AMQP-cpp" "alpha")
