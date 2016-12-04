list(APPEND RINGBUFFER_READER_SRC "reader.cc")
add_executable("reader" ${RINGBUFFER_READER_SRC})
target_link_libraries("reader" "alpha")
