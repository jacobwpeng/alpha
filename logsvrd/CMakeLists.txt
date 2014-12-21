set(BIN "logsvrd")

include_directories(${PROJECT_SOURCE_DIR})

add_executable(${BIN} logsvrd.cc)
target_link_libraries(${BIN} "alpha-bus" "alpha-logger")

add_executable("example-logsvrd-client" logsvrd-client.cc)
target_link_libraries("example-logsvrd-client" alpha-bus alpha-logger)
