set(LIB "alpha-logger")

list(APPEND srcs 
        logger_streambuf.cc 
        logger_stream.cc
        logger_formatter.cc
        log_file.cc
        logger.cc 
    )

add_library(${LIB} ${srcs})
add_executable("example-logger" example.cc)
target_link_libraries("example-logger" ${LIB})
