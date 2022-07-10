####################################################################################################
# A shitty imitation "C library" to get a lot of common stuff covered. Most of the code for this
# is straight up copied from OpenBSD's libc.
####################################################################################################
add_library(libc STATIC
    Sources/stdlib/abort.cpp

    Sources/string/memchr.c
    Sources/string/memcmp.c
    Sources/string/memcpy.c
    Sources/string/memmove.c
    Sources/string/memset.c

    Sources/string/strcmp.c
    Sources/string/strncpy.c
    Sources/string/strncmp.c
    Sources/string/strcpy.c
    Sources/string/strcspn.c
    Sources/string/strlen.c
    Sources/string/strspn.c

    Sources/aeabi/aeabi_memcpy.c
    Sources/aeabi/aeabi_memcpy.S

    Sources/malloc/init.c
    Sources/malloc/malloc_api.c

    Sources/math/trig.c

    Sources/runtime/init.c
)

target_include_directories(libc PUBLIC ${CMAKE_CURRENT_LIST_DIR}/Includes)

#########################
# fetch umm_malloc, used as a heap implementation
FetchContent_Declare(umm_malloc
    GIT_REPOSITORY https://github.com/rhempel/umm_malloc.git
    GIT_TAG v1.1.2
)
FetchContent_MakeAvailable(umm_malloc)

add_library(umm_malloc STATIC
    ${umm_malloc_SOURCE_DIR}/src/umm_info.c
    ${umm_malloc_SOURCE_DIR}/src/umm_integrity.c
    ${umm_malloc_SOURCE_DIR}/src/umm_malloc.c
    ${umm_malloc_SOURCE_DIR}/src/umm_poison.c
)

target_include_directories(umm_malloc PUBLIC ${umm_malloc_SOURCE_DIR}/src)
target_include_directories(umm_malloc PRIVATE ${CMAKE_CURRENT_LIST_DIR}/Includes)
target_include_directories(umm_malloc PRIVATE ${CMAKE_CURRENT_LIST_DIR}/Sources/malloc)

target_link_libraries(libc PRIVATE umm_malloc)

