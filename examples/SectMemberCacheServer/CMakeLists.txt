set(PROG "SectMemberCacheServer")

list(APPEND SECT_MEMBER_CACHE_SERVER_SVRD_SRCS
  "SectMemberCacheServerApp.cc"
  "SectMemberCacheServerMain.cc"
)

set(SECT_MEMBER_CACHE_SERVER_PROTO_LIB "SectMemberCacheServerProto")
add_subdirectory(proto)

add_executable(${PROG} ${SECT_MEMBER_CACHE_SERVER_SVRD_SRCS})
target_link_libraries(${PROG} ${SECT_MEMBER_CACHE_SERVER_PROTO_LIB} "alpha" "libprotobuf.a" "pthread")
