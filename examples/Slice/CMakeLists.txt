set(PROG "example_slice")
list(APPEND EXAMPLE_SLICE_SRCS "main.cc")
add_executable(${PROG} ${EXAMPLE_SLICE_SRCS})
target_link_libraries(${PROG} "alpha")
