#file(GLOB TOKYO_TYRANT_CLIENT_EXAMPLE_SRCS "*.cc")
list(APPEND SIMPLE_HTTP_SERVER_EXAMPLE_SRCS "main.cc")
add_executable("simple_http_server" ${SIMPLE_HTTP_SERVER_EXAMPLE_SRCS})
target_link_libraries("simple_http_server" "alpha")
