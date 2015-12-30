set(PROG "ThronesBattleSvrd")

list(APPEND THRONES_BATTLE_SVRD_SRCS
  "fightsvrd.pb.cc"
  "feedssvrd.pb.cc"
  "ThronesBattleSvrd.pb.cc"
  "ThronesBattleSvrdDef.cc"
  "ThronesBattleSvrdConf.cc"
  "ThronesBattleSvrdMessageDispatcher.cc"
  "ThronesBattleSvrdTaskBroker.cc"
  "ThronesBattleSvrdApp.cc"
  "ThronesBattleSvrdMain.cc"
)

#set(PROTOFILES "protofiles")
#set(SOURCE_FILE_DIR "${PROJECT_SOURCE_DIR}")
#add_custom_command(OUTPUT
#    ${SOURCE_FILE_DIR}/ThronesBattle.pb.h
#    ${SOURCE_FILE_DIR}/ThronesBattle.pb.cc
#    COMMAND protoc -I=${SOURCE_FILE_DIR} --cpp_out=${SOURCE_FILE_DIR}
#    ${SOURCE_FILE_DIR}/ThronesBattle.proto
#    DEPENDS ${SOURCE_FILE_DIR}/ThronesBattle.proto
#)
#add_custom_target(${PROTOFILES} ALL DEPENDS ${SOURCE_FILE_DIR}/ThronesBattle.pb.h)

find_library(PROTOBUF "libprotobuf.a")
find_library(PTHREAD "pthread")
add_executable(${PROG} ${THRONES_BATTLE_SVRD_SRCS})
target_link_libraries(${PROG} "alpha" ${PROTOBUF} ${PTHREAD})
#add_dependencies(${PROG} ${PROTOFILES})
