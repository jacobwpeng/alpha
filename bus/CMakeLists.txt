set(LIB "alpha-bus")

list(APPEND srcs 
        mmap_file.cc
        ring_buffer.cc
        process_bus.cc
    )

add_library(${LIB} ${srcs})
add_executable("example-bus" example.cc)
target_link_libraries("example-bus" ${LIB})
