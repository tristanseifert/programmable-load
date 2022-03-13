####################################################################################################
# Programmable load firmware
#
# This defines the main firmware target, which produces the actual binary image that's flashed into
# the microcontroller.
####################################################################################################
###############
# Set up the CMake project and include some plugins
cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
project(programmable-load VERSION 2.0 LANGUAGES ASM C CXX)


include(ExternalProject)
include(FetchContent)

###############
# Get version information from Git and some additional build info
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND git log -1 --format=%h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
cmake_host_system_information(RESULT BUILD_HOSTNAME QUERY FQDN)
set(BUILD_USERNAME $ENV{USER})

# Generate a C++ file containing the build info
configure_file(${CMAKE_CURRENT_LIST_DIR}/Sources/BuildInfo.cpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/BuildInfo.cpp)
set(BuildInfoFile "${CMAKE_CURRENT_BINARY_DIR}/BuildInfo.cpp")

###############
# Set warning levels and language version
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wmissing-declarations -Wformat=2 -fdiagnostics-color=always
    -ftls-model=initial-exec -Wundef -Wcast-qual -Wwrite-strings)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options(-Werror -Wimplicit-fallthrough -Wno-deprecated-copy -Wno-address-of-packed-member
        -Wno-expansion-to-defined -Wno-undef -Wno-unused-private-field -Wno-deprecated-volatile)
endif()

###############
# Define the target/system flags

###############
# Set up the primary firmware target
add_executable(firmware
    Sources/Init/Vectors.c
    Sources/Init/Startup.cpp
    Sources/Main.cpp
    Sources/Log/Logger.cpp
    Sources/Log/TraceSWO.cpp
    ${BuildInfoFile}
)

# Set defines for the platform
add_compile_definitions(__SAME51J20A__ __FPU_USED)

# Include directories
target_include_directories(firmware PUBLIC Includes)
target_include_directories(firmware PRIVATE Sources)

# Configure it to build a static image
target_compile_options(firmware PRIVATE -ffreestanding -flto -fno-exceptions -fno-unwind-tables)
target_link_options(firmware PRIVATE --static -nostartfiles -nostdlib -ffreestanding -flto -fno-exceptions -fno-unwind-tables)

# Use the appropriate linker script
set_target_properties(firmware PROPERTIES OUTPUT_NAME "programmable-load-firmware")
set_target_properties(firmware PROPERTIES SUFFIX ".elf")

set_target_properties(firmware PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/same51j20a_flash.ld)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_link_options(firmware PRIVATE "-Wl,-T${CMAKE_CURRENT_LIST_DIR}/same51j20a_flash.ld")
endif()

###############
# Build various external components

# CMSIS 5
FetchContent_Declare(cmsis_5
    GIT_REPOSITORY https://github.com/ARM-software/CMSIS_5.git
    GIT_TAG 5.8.0
)
FetchContent_MakeAvailable(cmsis_5)

set(CMSIS_INCLUDE_DIR ${cmsis_5_SOURCE_DIR}/CMSIS/Core/Include)
target_include_directories(firmware PRIVATE ${CMSIS_INCLUDE_DIR})

# C library
add_subdirectory(External/libc)
target_link_libraries(firmware PRIVATE libc)

# Embedded template library
option(NO_STL "" ON)

FetchContent_Declare(etl
    GIT_REPOSITORY https://github.com/ETLCPP/etl.git
    GIT_TAG 20.25.0
)
FetchContent_MakeAvailable(etl)
target_compile_options(etl INTERFACE -DETL_NO_STL)

target_link_libraries(firmware PRIVATE etl)

# Atmel-provided vendor code: headers and low level drivers
add_subdirectory(External/atmel)

target_include_directories(firmware PUBLIC External/atmel/Includes)
target_link_libraries(firmware PRIVATE vendor-atmel)

# printf
set(BUILD_STATIC_LIBRARY "" ON)
set(SUPPORT_WRITEBACK_SPECIFIER "" OFF)

FetchContent_Declare(printf_library
    GIT_REPOSITORY https://github.com/eyalroz/printf.git
    GIT_TAG v6.0.0
)
FetchContent_MakeAvailable(printf_library)

target_compile_options(printf PRIVATE -Wno-implicit-int-float-conversion)
target_link_libraries(firmware PRIVATE printf)

###############
# Produce a firmware image from the primary target, ready to be flashed into the microcontroller;
# this just pulls out the .text segment out of the executable.

# after building, extract the .text section and save it as a raw binary; and print the sizes
find_program(CMAKE_SIZE size)
find_program(CMAKE_OBJCOPY objcopy)

add_custom_command(TARGET firmware POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:firmware>
)
add_custom_command(TARGET firmware POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary --only-section=.text --only-section=.data $<TARGET_FILE:firmware> ${CMAKE_CURRENT_BINARY_DIR}/firmware.bin)