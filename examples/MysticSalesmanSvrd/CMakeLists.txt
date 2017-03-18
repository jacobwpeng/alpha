set(PROG "MysticSalesmanSvrd")

list(APPEND MYSTIC_SALESMAN_SVRD_SRCS
  "MysticSalesmanSvrdApp.cc"
  "MysticSalesmanSvrdMain.cc"
)

set(MYSTIC_SALESMAN_PROTO_LIB "MysticSalesmanProto")
add_subdirectory(proto)

add_executable(${PROG} ${MYSTIC_SALESMAN_SVRD_SRCS})
target_link_libraries(${PROG} ${MYSTIC_SALESMAN_PROTO_LIB} "alpha" "protobuf" "pthread")
