list(APPEND AMQP_CPP_SRCS
  "FieldValue.cc"
  "FieldTable.cc"
  "MethodArgTypes.cc"
  "CodedWriter.cc"
  "CodedInputStream.cc"
  "CodedOutputStream.cc"
  "CodecEnv.cc"
  "EncodeUnit.cc"
  "DecodeUnit.cc"
  "Frame.cc"
  "FrameCodec.cc"
  "MethodPayloadCodec.cc"
  "ConnectionParameters.cc"
  "Channel.cc"
  "Connection.cc"
  #"FSM.cc"
  #"ConnectionEstablishFSM.cc"
  #"ConnectionCloseFSM.cc"
  #"Channel.cc"
  "ConnectionMgr.cc"
  "main.cc"
)
add_executable("AMQP-cpp" ${AMQP_CPP_SRCS})
target_link_libraries("AMQP-cpp" "alpha")
