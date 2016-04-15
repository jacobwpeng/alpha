set(PROG "SectInfoServer")

list(APPEND SECT_INFO_SVRD_SRCS
  "SectInfoServerApp.cc"
  "SectInfoServerMain.cc"
)

set(SECT_INFO_PROTO_LIB "SectInfoProto")
add_subdirectory(proto)

add_executable(${PROG} ${SECT_INFO_SVRD_SRCS})
target_link_libraries(${PROG} "alpha" "libprotobuf.a" "pthread" ${SECT_INFO_PROTO_LIB})
