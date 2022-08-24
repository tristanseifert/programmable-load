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

# don't allow upcasts to double
add_compile_options(-Wdouble-promotion)

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
add_firmware_executable(NAME firmware FILENAME "programmable-load-firmware"
    LDSCRIPT ${CMAKE_CURRENT_LIST_DIR}/stm32mp15x.ld
    LDMAP ${CMAKE_CURRENT_BINARY_DIR}/output.map
    SOURCES
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
    Sources/App/Control/Hardware.cpp
    Sources/App/Control/Task.cpp
    Sources/App/Rpmsg/Task.cpp
)

target_include_directories(firmware PUBLIC Includes)
target_include_directories(firmware PRIVATE Sources)

####################################################################################################
# Configure and include various external components
target_link_libraries(firmware PRIVATE embedded-fw-base::tinycbor)

# FreeRTOS
set(FREERTOS_CONFIG_FILE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/Includes/FreeRTOS/" CACHE STRING "")
target_link_libraries(firmware PRIVATE embedded-fw-base::FreeRTOS)

# OpenAMP (all components)
add_subdirectory(External/openamp EXCLUDE_FROM_ALL)
target_link_libraries(firmware PRIVATE open_amp-static)
