####################################################################################################
# OpenAMP (used for coprocessor stuff)
####################################################################################################

# libmetal: hardware abstraction layer for OpenAMP
set(PROJECT_SYSTEM "FreeRTOS" CACHE INTERNAL "" FORCE)
set(MACHINE "stm32mp15x" CACHE INTERNAL "" FORCE)

set(WITH_DOC OFF CACHE INTERNAL "" FORCE)
set(WITH_EXAMPLES OFF CACHE INTERNAL "" FORCE)
set(WITH_TESTS OFF CACHE INTERNAL "" FORCE)
set(WITH_SHARED_LIB OFF CACHE INTERNAL "" FORCE)
set(WITH_STATIC_LIB ON CACHE INTERNAL "" FORCE)

set(WITH_DEFAULT_LOGGER OFF CACHE INTERNAL "" FORCE)

FetchContent_Declare(libmetal
    GIT_REPOSITORY https://github.com/OpenAMP/libmetal.git
    GIT_TAG v2022.04.0
)
FetchContent_MakeAvailable(libmetal)

target_link_libraries(metal-static PRIVATE libc freertos_kernel)
target_include_directories(metal-static PUBLIC ${CMAKE_CURRENT_LIST_DIR}/libmetal/machine)

# no support for 64-bit atomics
target_compile_definitions(metal-static PUBLIC -DNO_ATOMIC_64_SUPPORT)

#########################
# OpenAMP: remote processor communication and such friends
set(WITH_APPS OFF CACHE INTERNAL "" FORCE)

set(LIBMETAL_LIB "$<TARGET_FILE:metal-static>")
set(LIBMETAL_INCLUDE_DIR ${libmetal_BINARY_DIR}/lib/include)

FetchContent_Declare(openamp
    GIT_REPOSITORY https://github.com/OpenAMP/open-amp.git
    GIT_TAG v2022.04.0
)
FetchContent_MakeAvailable(openamp)

target_link_libraries(open_amp-static PRIVATE libc freertos_kernel)

target_include_directories(open_amp-static PUBLIC ${openamp_SOURCE_DIR}/lib/include ${LIBMETAL_INCLUDE_DIR} ${CMAKE_CURRENT_LIST_DIR}/libmetal/machine)
target_link_libraries(open_amp-static PRIVATE metal-static)

#########################
# machine support
add_library(libmetal-machine STATIC
    ${CMAKE_CURRENT_LIST_DIR}/libmetal/machine/stm32mp15x/sys.c
)
target_link_libraries(libmetal-machine PRIVATE open_amp-static)
target_link_libraries(open_amp-static PRIVATE libmetal-machine)

