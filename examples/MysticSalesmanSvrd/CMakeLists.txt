set(PROG "MysticSalesmanSvrd")

list(APPEND MYSTIC_SALESMAN_SVRD_SRCS
  "MysticSalesmanSvrd.pb.cc"
  "MysticSalesmanSvrdApp.cc"
  "MysticSalesmanSvrdMain.cc"
)

find_library(PROTOBUF "libprotobuf.a")
add_executable(${PROG} ${MYSTIC_SALESMAN_SVRD_SRCS})
target_link_libraries(${PROG} "alpha" ${PROTOBUF})
