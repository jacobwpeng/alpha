file(GLOB SRCS "*.cc")
list(APPEND COROUTINE_EXAMPLE_SRCS "player.cc" "main.cc")
add_executable("main" ${COROUTINE_EXAMPLE_SRCS})
target_link_libraries("main" "alpha")
