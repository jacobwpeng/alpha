list(APPEND REGION_BASED_CONTAINER_EXAMPLE_SRCS "main.cc")
add_executable("region_based_container_example" ${REGION_BASED_CONTAINER_EXAMPLE_SRCS})
target_link_libraries("region_based_container_example" "alpha")
