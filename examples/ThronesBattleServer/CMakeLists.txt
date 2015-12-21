set(PROG "ThronesBattleSvrd")

list(APPEND THRONES_BATTLE_SVRD_SRCS
  "fightsvrd.pb.cc"
  "ThronesBattleSvrdDef.cc"
  "ThronesBattleSvrdConf.cc"
  "ThronesBattleSvrdTaskBroker.cc"
  "ThronesBattleSvrdApp.cc"
  "ThronesBattleSvrdMain.cc"
)

find_library(PROTOBUF "libprotobuf.a")
find_library(PTHREAD "pthread")
add_executable(${PROG} ${THRONES_BATTLE_SVRD_SRCS})
target_link_libraries(${PROG} "alpha" ${PROTOBUF} ${PTHREAD})
