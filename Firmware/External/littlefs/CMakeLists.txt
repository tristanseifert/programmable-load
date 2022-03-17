####################################################################################################
# Compile
####################################################################################################
# get the code from git repo
FetchContent_Declare(littlefs
    GIT_REPOSITORY https://github.com/littlefs-project/littlefs.git
    GIT_TAG v2.4.1
)
FetchContent_MakeAvailable(littlefs)

# create a static library from its sources
add_library(littlefs STATIC
    ${littlefs_SOURCE_DIR}/lfs.c
    ${littlefs_SOURCE_DIR}/lfs_util.c
)
target_link_libraries(littlefs PRIVATE libc)

target_include_directories(littlefs PRIVATE Includes)
target_include_directories(littlefs PUBLIC ${littlefs_SOURCE_DIR})

# disable a few warnings
target_compile_options(littlefs PRIVATE -Wno-cast-qual -Wno-unused-function)

# specify custom configuration: we don't use dynamic allocation
target_compile_definitions(littlefs PRIVATE -DLFS_NO_MALLOC -DLFS_CONFIG=LfsConfigCustom.h)

# expose the "scary" libc functions
target_compile_definitions(littlefs PRIVATE -DWITH_SCARY_FUNCTIONS)
