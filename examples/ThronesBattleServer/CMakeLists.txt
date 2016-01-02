set(PROG "ThronesBattleSvrd")

list(APPEND THRONES_BATTLE_SVRD_SRCS
  "ThronesBattleSvrdRankVector.cc"
  "ThronesBattleSvrdDef.cc"
  "ThronesBattleSvrdConf.cc"
  "ThronesBattleSvrdMessageDispatcher.cc"
  "ThronesBattleSvrdTaskBroker.cc"
  "ThronesBattleSvrdApp.cc"
  "ThronesBattleSvrdMain.cc"
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ext)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/proto)

set(THRONES_BATTLE_PROTO_LIB "ThronesBattleProto")
add_subdirectory(proto)

find_library(PROTOBUF "libprotobuf.a")
find_library(PTHREAD "pthread")
add_executable(${PROG} ${THRONES_BATTLE_SVRD_SRCS})
target_link_libraries(${PROG} "alpha" ${PROTOBUF} ${PTHREAD} ${THRONES_BATTLE_PROTO_LIB})
