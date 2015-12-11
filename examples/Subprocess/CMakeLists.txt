set(PROG "subprocess")
list(APPEND SUBPROCESS_PROG_SRCS "main.cc")
add_executable(${PROG} ${SUBPROCESS_PROG_SRCS})
target_link_libraries(${PROG} "alpha")
