set(PROG "SectMemberCacheServer")

list(APPEND SECT_MEMBER_CACHE_SERVER_SVRD_SRCS
  "SectMemberCacheServerApp.cc"
  "SectMemberCacheServerMain.cc"
)

set(SECT_MEMBER_CACHE_SERVER_PROTO_LIB "SectMemberCacheServerProto")
add_subdirectory(proto)

list(APPEND LIBS
  "${SECT_MEMBER_CACHE_SERVER_PROTO_LIB}"
  "alpha"
  "libprotobuf.a"
  "pthread"
)

add_executable(${PROG} ${SECT_MEMBER_CACHE_SERVER_SVRD_SRCS})
if ("${RPATH}" STREQUAL "")
  target_link_libraries(${PROG} ${LIBS})
else()
target_link_libraries(${PROG}
  PRIVATE
  "-Wl,--rpath=${RPATH}"
  "-Wl,--dynamic-linker=${RPATH}/ld-linux-x86-64.so.2"
  ${LIBS})
endif()
