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

# external embedded base lib
set(EFWB_TARGET "stm32mp15x")

# XXX: figure out how to support floating point only mode
set(EFWB_CBOR_HAS_FLOAT NO)

FetchContent_Declare(
    fw-base
    GIT_REPOSITORY https://github.com/tristanseifert/embedded-fw-base.git
    GIT_TAG main
)
FetchContent_MakeAvailable(fw-base)
link_libraries(embedded-fw-base::libc)

###############
# Set up the primary firmware target
add_executable(firmware
    ${BuildInfoFile}
    Sources/Init/StackGuard.cpp
    Sources/Init/CppHeap.cpp
    Sources/Init/CppRuntime.cpp
    Sources/Main.cpp
    Sources/Rpc/Rpc.cpp
    Sources/Rpc/Mailbox.cpp
    Sources/Rpc/MessageHandler.cpp
    Sources/Rpc/OpenAmp.cpp
    Sources/Rpc/ResourceTable.cpp
    Sources/Rpc/Endpoints/Confd/Handler.cpp
    Sources/Rpc/Endpoints/Confd/Service.cpp
    Sources/Rpc/Endpoints/ResourceManager/Handler.cpp
    Sources/Rpc/Endpoints/ResourceManager/Service.cpp
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
    Sources/App/Rpmsg/Task.cpp
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
# C library
target_link_libraries(firmware PRIVATE embedded-fw-base::cmsis)

# libraries
target_link_libraries(firmware PRIVATE embedded-fw-base::etl)
target_link_libraries(firmware PRIVATE embedded-fw-base::tinycbor)

# ST-provided vendor code: headers and low level drivers
add_subdirectory(External/st)

target_include_directories(firmware PUBLIC External/st/Includes)
target_link_libraries(firmware PRIVATE vendor-st)

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
add_subdirectory(External/openamp EXCLUDE_FROM_ALL)
target_link_libraries(firmware PRIVATE open_amp-static)
