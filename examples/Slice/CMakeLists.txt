list(APPEND SLICE_EXAMPLE_SRCS "main.cc")
add_executable("slice_example" ${SLICE_EXAMPLE_SRCS})
target_link_libraries("slice_example" "alpha")
