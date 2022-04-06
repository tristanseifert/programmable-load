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
    Sources/Init/Vectors.c
    Sources/Init/Startup.cpp
    Sources/Init/StackGuard.cpp
    Sources/Init/CppHeap.cpp
    Sources/Init/FaultHandlers.cpp
    Sources/Main.cpp
    Sources/Log/Logger.cpp
    Sources/Log/TraceSWO.cpp
    Sources/Rtos/Idle.cpp
    Sources/Rtos/Memory.cpp
    Sources/Rtos/Start.cpp
    Sources/Util/InventoryRom.cpp
    Sources/Drivers/ClockMgmt.cpp
    Sources/Drivers/Gpio.cpp
    Sources/Drivers/ExternalIrq.cpp
    Sources/Drivers/SercomBase.cpp
    Sources/Drivers/I2CBus.cpp
    Sources/Drivers/I2C.cpp
    Sources/Drivers/Spi.cpp
    Sources/Drivers/TimerCounter.cpp
    Sources/Drivers/I2CDevice/Common.cpp
    Sources/Drivers/I2CDevice/PCA9543A.cpp
    Sources/Drivers/I2CDevice/EMC2101.cpp
    Sources/Drivers/I2CDevice/AT24CS32.cpp
    Sources/Drivers/I2CDevice/XRA1203.cpp
    Sources/Drivers/I2CDevice/PCA9955B.cpp
    Sources/Drivers/I2CDevice/PI4IOE5V9536.cpp
    Sources/Usb/Descriptors.cpp
    Sources/Usb/Init.cpp
    Sources/Usb/Task.cpp
    Sources/App/Main/Hardware.cpp
    Sources/App/Main/Task.cpp
    Sources/App/Pinball/Hardware.cpp
    Sources/App/Pinball/Task.cpp
    Sources/App/Pinball/FrontIo/HmiDriver.cpp
    Sources/App/Thermal/Hardware.cpp
    Sources/App/Thermal/Task.cpp
    Sources/App/Control/Hardware.cpp
    Sources/App/Control/Task.cpp
    ${BuildInfoFile}
)

# Set defines for the platform
add_compile_definitions(__SAME51J20A__ __FPU_PRESENT __FPU_USED)

# Include directories
target_include_directories(firmware PUBLIC Includes)
target_include_directories(firmware PRIVATE Sources)

# Configure it to build a static image
target_compile_options(firmware PRIVATE -flto)
target_link_options(firmware PRIVATE --static -nostartfiles -nostdlib -ffreestanding -flto)

# Use the appropriate linker script
set_target_properties(firmware PROPERTIES OUTPUT_NAME "programmable-load-firmware")
set_target_properties(firmware PROPERTIES SUFFIX ".elf")

set_target_properties(firmware PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/same51j20a_flash.ld)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_link_options(firmware PRIVATE "-Wl,-T${CMAKE_CURRENT_LIST_DIR}/same51j20a_flash.ld")
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
    GIT_TAG 5.8.0
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
add_subdirectory(External/libc)
target_link_libraries(firmware PRIVATE libc)

###############
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
# give it a dummy heap file
set(FREERTOS_HEAP ${CMAKE_CURRENT_LIST_DIR}/Includes/BuildInfo.h CACHE STRING "")
set(FREERTOS_PORT GCC_ARM_CM4F CACHE STRING "")

FetchContent_Declare(FreeRTOS-Kernel
    GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS-Kernel.git
    # CMake support not yet in a release version
    GIT_TAG main
)
FetchContent_MakeAvailable(FreeRTOS-Kernel)

target_link_libraries(firmware PRIVATE freertos_kernel)
target_link_libraries(freertos_kernel libc vendor-atmel)

###############
# TinyUSB (Generic USB stack)
set(TINYUSB_CONFIG_DIR "${CMAKE_CURRENT_LIST_DIR}/Includes/TinyUSB/")

add_subdirectory(External/tinyusb)

target_link_libraries(firmware PRIVATE tinyusb)
target_link_libraries(tinyusb libc vendor-atmel freertos_kernel)

target_link_libraries(firmware PRIVATE tinyusb-platform-atsamd)
target_link_libraries(tinyusb-platform-atsamd libc vendor-atmel)

###############
# LittleFS (for NOR flash)
add_subdirectory(External/littlefs)
target_link_libraries(firmware PRIVATE littlefs)

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
