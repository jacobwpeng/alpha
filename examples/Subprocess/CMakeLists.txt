set(PROG "example_subprocess")
list(APPEND EXAMPLE_SUBPROCESS_SRCS "main.cc")
add_executable(${PROG} ${EXAMPLE_SUBPROCESS_SRCS})
target_link_libraries(${PROG} "alpha")
