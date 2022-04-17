####################################################################################################
# A shitty imitation "C library" to get a lot of common stuff covered. Most of the code for this
# is straight up copied from OpenBSD's libc.
####################################################################################################
add_library(libc STATIC
    Sources/stdlib/abort.cpp

    Sources/string/memcmp.c
    Sources/string/memcpy.c
    Sources/string/memset.c

    Sources/string/strcmp.c
    Sources/string/strncmp.c
    Sources/string/strcpy.c
    Sources/string/strcspn.c
    Sources/string/strlen.c
    Sources/string/strspn.c

    Sources/aeabi/aeabi_memcpy.c
    Sources/aeabi/aeabi_memcpy.S

    Sources/math/trig.c
)

target_include_directories(libc PUBLIC ${CMAKE_CURRENT_LIST_DIR}/Includes)
