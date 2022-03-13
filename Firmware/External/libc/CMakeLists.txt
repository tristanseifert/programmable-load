####################################################################################################
# A shitty imitation "C library" to get a lot of common stuff covered. Most of the code for this
# is straight up copied from OpenBSD's libc.
####################################################################################################
add_library(libc STATIC
    Sources/string/memcpy.c

    Sources/aeabi/aeabi_memcpy.c
    Sources/aeabi/aeabi_memcpy.S
)

target_include_directories(libc PUBLIC ${CMAKE_CURRENT_LIST_DIR}/Includes)
