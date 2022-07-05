####################################################################################################
# Programmable load firmware
#
# This defines the main firmware target which produces the ELF that's loaded by the remoteproc
# framework into the processor's memory spaces.
####################################################################################################
###############
# Set up the CMake project and include some plugins
cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
project(programmable-load VERSION 0.1 LANGUAGES ASM C CXX)


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
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wmissing-declarations -Wformat=2 -fdiagnostics-color=always
    -ftls-model=initial-exec -Wundef -Wcast-qual -Wwrite-strings)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options(-Werror -Wimplicit-fallthrough -Wno-deprecated-copy -Wno-address-of-packed-member
        -Wno-expansion-to-defined -Wno-undef -Wno-unused-private-field -Wno-deprecated-volatile)
endif()

###############
# Define the target/system flags
# place functions into their own sections (for LTO)
add_compile_options(-ffunction-sections -fdata-sections -fno-common)
# disable exceptions, stack unwinding
add_compile_options(-ffreestanding -fno-exceptions -fno-unwind-tables -fno-rtti)
# note that we can only support single precision float: warn when promoting to double
add_compile_options(-Wdouble-promotion)
# no need for teardown code/destructors
add_compile_options(-fno-use-cxa-atexit)
# enable stack guards
add_compile_options(-fstack-protector-strong)

###############
# Set up the primary firmware target
add_executable(firmware
    ${BuildInfoFile}
    Sources/Init/StackGuard.cpp
    Sources/Init/CppHeap.cpp
    #Sources/Init/CppRuntime.cpp
    #Sources/Init/FaultHandlers.cpp
    Sources/Main.cpp
    Sources/Rpc/ResourceTable.cpp
    Sources/Log/Logger.cpp
    Sources/Rtos/Idle.cpp
    Sources/Rtos/Memory.cpp
    Sources/Rtos/Start.cpp
    Sources/Util/InventoryRom.cpp
    Sources/Util/Hash.cpp
    #Sources/Util/HwInfo.cpp
    Sources/Drivers/Gpio.cpp
    Sources/Drivers/Random.cpp
    Sources/Drivers/Watchdog.cpp
    Sources/Supervisor/Supervisor.cpp
    Sources/Supervisor/Task.cpp
)

# Set defines for the platform
add_compile_definitions(CORE_CM4 STM32MP151Axx)
# add_compile_definitions(__FPU_PRESENT __FPU_USED)

# Include directories
target_include_directories(firmware PUBLIC Includes)
target_include_directories(firmware PRIVATE Sources)

# Configure it to build a static image
target_compile_options(firmware PRIVATE -flto)
target_link_options(firmware PRIVATE --static -nostartfiles -nostdlib -ffreestanding -flto)

# Use the appropriate linker script
set_target_properties(firmware PROPERTIES OUTPUT_NAME "programmable-load-firmware")
set_target_properties(firmware PROPERTIES SUFFIX ".elf")

set_target_properties(firmware PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/stm32mp15x.ld)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_link_options(firmware PRIVATE "-Wl,-T${CMAKE_CURRENT_LIST_DIR}/stm32mp15x.ld")
endif()

####################################################################################################
# Build various external components and libraries
#
# Most of these are declared in external projects, fetched as needed, but some are in the source
# tree and included here.

###############
# CMSIS 5
FetchContent_Declare(cmsis_5
    GIT_REPOSITORY https://github.com/ARM-software/CMSIS_5.git
    GIT_TAG 5.9.0
)
FetchContent_MakeAvailable(cmsis_5)

set(CMSIS_INCLUDE_DIR ${cmsis_5_SOURCE_DIR}/CMSIS/Core/Include)

# let everything we compile pull in the CMSIS headers
include_directories(${CMSIS_INCLUDE_DIR})

###############
# C library
#
# This is our loose conglomeration of C/C++ code flying in close formation that resembles a C
# library. It's most definitely not standards compliant, but sufficient to get embedded stuff
# going.
add_subdirectory(External/libc EXCLUDE_FROM_ALL)
target_link_libraries(firmware PRIVATE libc)

###############
# Embedded template library
option(NO_STL "" ON)

FetchContent_Declare(etl
    GIT_REPOSITORY https://github.com/ETLCPP/etl.git
    GIT_TAG 20.29.3
)
FetchContent_MakeAvailable(etl)
target_compile_options(etl INTERFACE -DETL_NO_STL -DETL_NO_CPP_NAN_SUPPORT)

target_link_libraries(firmware PRIVATE etl)

# ST-provided vendor code: headers and low level drivers
add_subdirectory(External/st)

target_include_directories(firmware PUBLIC External/st/Includes)
target_link_libraries(firmware PRIVATE vendor-st)

###############
# printf
set(BUILD_STATIC_LIBRARY "" ON)
set(SUPPORT_WRITEBACK_SPECIFIER "" OFF)

set(SUPPORT_DECIMAL_SPECIFIERS "" OFF)
set(SUPPORT_EXPONENTIAL_SPECIFIERS "" OFF)

FetchContent_Declare(printf_library
    GIT_REPOSITORY https://github.com/eyalroz/printf.git
    GIT_TAG v6.0.0
)
FetchContent_MakeAvailable(printf_library)

target_compile_options(printf PRIVATE -Wno-implicit-int-float-conversion)
target_link_libraries(firmware PRIVATE printf)
target_compile_definitions(firmware PUBLIC -DPRINTF_ALIAS_STANDARD_FUNCTION_NAMES=1)

###############
# FreeRTOS
set(FREERTOS_CONFIG_FILE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/Includes/FreeRTOS/" CACHE STRING "")
# wrap our C library's allocator
set(FREERTOS_HEAP "3" CACHE STRING "")
set(FREERTOS_PORT GCC_ARM_CM4F CACHE STRING "")

FetchContent_Declare(FreeRTOS-Kernel
    GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS-Kernel.git
    # CMake support not yet in a release version
    GIT_TAG main
)
FetchContent_MakeAvailable(FreeRTOS-Kernel)

target_link_libraries(freertos_kernel libc vendor-st)
target_link_libraries(firmware PRIVATE freertos_kernel)

###############
# CMSIS DSP
#
# This is primarily used for some fast math functions, which the C library then re-exports as
# builtin C library functions.
set(ROOT ${cmsis_5_SOURCE_DIR})
set(DSP ${cmsis_5_SOURCE_DIR}/CMSIS/DSP)
set(ARM_CPU "cortex-m4f")

# hack to get cmake step to work
function(compilerSpecificCompileOptions PROJECTNAME ROOT)
endfunction()

# disable a lot of code
set(DISABLEFLOAT16 ON CACHE INTERNAL "" FORCE)
set(COMPLEXMATH OFF CACHE INTERNAL "" FORCE)
set(CONTROLLER OFF CACHE INTERNAL "" FORCE)
set(FILTERING OFF CACHE INTERNAL "" FORCE)
set(MATRIX OFF CACHE INTERNAL "" FORCE)
set(TRANSFORM OFF CACHE INTERNAL "" FORCE)
set(SVM OFF CACHE INTERNAL "" FORCE)
set(BAYES OFF CACHE INTERNAL "" FORCE)
set(DISTANCE OFF CACHE INTERNAL "" FORCE)
set(INTERPOLATION OFF CACHE INTERNAL "" FORCE)
set(QUATERNIONMATH OFF CACHE INTERNAL "" FORCE)

list(APPEND CMAKE_MODULE_PATH ${DSP})

# this is a disgusting hack, we should figure out how to change it out
add_compile_options(-Wno-error)
include_directories(External/libc/Includes)

add_subdirectory(${DSP}/Source bin_dsp EXCLUDE_FROM_ALL)

target_link_libraries(libc PRIVATE CMSISDSP)

###############
add_subdirectory(External/openamp EXCLUDE_FROM_ALL)

target_link_libraries(firmware PRIVATE open_amp-static)

###############
# TinyCBOR
#
# A C/C++ library to encode objects as CBOR (a binary object representation)
add_subdirectory(External/tinycbor EXCLUDE_FROM_ALL)
target_link_libraries(tinycbor PRIVATE libc)

target_link_libraries(firmware PRIVATE tinycbor)

###############
# Print the size of the firmware after building
find_program(CMAKE_SIZE size)

add_custom_command(TARGET firmware POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:firmware>
)
