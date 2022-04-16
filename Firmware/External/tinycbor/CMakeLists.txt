####################################################################################################
# TinyCBOR
#
# Intel's CBOR encoding library, specifically designed for embedded land.
####################################################################################################
# get code from git
FetchContent_Declare(tinycbor
    GIT_REPOSITORY https://github.com/intel/tinycbor.git
    GIT_TAG v0.6.0
)
FetchContent_MakeAvailable(tinycbor)

# create a static library
add_library(tinycbor STATIC
    ${tinycbor_SOURCE_DIR}/src/cborencoder.c
    ${tinycbor_SOURCE_DIR}/src/cborencoder_close_container_checked.c
    ${tinycbor_SOURCE_DIR}/src/cborencoder_float.c
    ${tinycbor_SOURCE_DIR}/src/cborerrorstrings.c
    ${tinycbor_SOURCE_DIR}/src/cborparser.c
    ${tinycbor_SOURCE_DIR}/src/cborparser_dup_string.c
    ${tinycbor_SOURCE_DIR}/src/cborparser_float.c
    ${tinycbor_SOURCE_DIR}/src/cborpretty.c
    #    ${tinycbor_SOURCE_DIR}/src/cborpretty_stdio.c
    #    ${tinycbor_SOURCE_DIR}/src/cbortojson.c
    ${tinycbor_SOURCE_DIR}/src/cborvalidation.c
)

target_link_libraries(tinycbor PRIVATE libc)
target_include_directories(tinycbor PUBLIC ${tinycbor_SOURCE_DIR}/src)

