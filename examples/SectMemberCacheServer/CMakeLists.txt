set(PROG "example_sect_member_cache_server")

list(APPEND SRCS
  "SectMemberCacheServerApp.cc"
  "SectMemberCacheServerMain.cc"
)

set(SECT_MEMBER_CACHE_SERVER_PROTO_LIB "SectMemberCacheServerProto")
add_subdirectory(proto)

list(APPEND LIBS
  ${SECT_MEMBER_CACHE_SERVER_PROTO_LIB}
  alpha
  protobuf
  pthread
)

add_executable(${PROG} ${SRCS})
target_link_libraries(${PROG} ${LIBS})
