set(BIN "logsvrd")

find_library(pthread pthread)
add_executable(${BIN} logsvrd.cc)
target_link_libraries(${BIN} alpha-base alpha-bus alpha-logger ${pthread})

add_executable("example-logsvrd-client" logsvrd-client.cc)
target_link_libraries("example-logsvrd-client" alpha-base alpha-bus alpha-logger)
