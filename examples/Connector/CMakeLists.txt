set(PROG "connector_example")
list(APPEND CONNECTOR_EXAMPLE_SRCS "main.cc")
add_executable(${PROG} ${CONNECTOR_EXAMPLE_SRCS})
target_link_libraries(${PROG} "alpha")
