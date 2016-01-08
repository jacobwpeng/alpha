set(PROG "ThronesBattleSvrd")

list(APPEND THRONES_BATTLE_SVRD_SRCS
  "ThronesBattleSvrdRankVector.cc"
  "ThronesBattleSvrdDef.cc"
  "ThronesBattleSvrdConf.cc"
  "ThronesBattleSvrdMessageDispatcher.cc"
  "ThronesBattleSvrdTaskBroker.cc"
  "ThronesBattleSvrdFeedsChannel.cc"
  "ThronesBattleSvrdApp.cc"
  "ThronesBattleSvrdMain.cc"
)

set(THRONES_BATTLE_PROTO_LIB "ThronesBattleProto")
add_subdirectory(proto)

add_executable(${PROG} ${THRONES_BATTLE_SVRD_SRCS})
target_link_libraries(${PROG} "alpha" "libprotobuf.a" "pthread" ${THRONES_BATTLE_PROTO_LIB})
