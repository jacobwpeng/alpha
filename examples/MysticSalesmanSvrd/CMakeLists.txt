set(PROG "MysticSalesmanSvrd")

list(APPEND MYSTIC_SALESMAN_SVRD_SRCS
  "MysticSalesmanSvrdApp.cc"
  "MysticSalesmanSvrdMain.cc"
)

set(MYSTIC_SALESMAN_PROTO_LIB "MysticSalesmanProto")
add_subdirectory(proto)

find_library(PROTOBUF "libprotobuf.a")
add_executable(${PROG} ${MYSTIC_SALESMAN_SVRD_SRCS})
target_link_libraries(${PROG} ${MYSTIC_SALESMAN_PROTO_LIB} "alpha" ${PROTOBUF} "pthread")
