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
target_include_directories(metal-static PRIVATE ${CMAKE_CURRENT_LIST_DIR}/libmetal/machine)

