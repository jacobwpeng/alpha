list(APPEND RANDOM_EXAMPLE_SRCS "main.cc")
add_executable("random_example" ${RANDOM_EXAMPLE_SRCS})
target_link_libraries("random_example" "alpha")
