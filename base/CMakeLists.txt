set(LIB "alpha-base")

list(APPEND srcs 
            slice.cc
    )

add_library(${LIB} ${srcs})
