####################################################################################################
# Programmable load interface library
####################################################################################################
# Find LibUSB
find_package(LibUSB)

# Define the library
add_library(libload STATIC
    Sources/LibLoad.cpp
    Sources/Device.cpp
    Sources/Usb.cpp
)
target_include_directories(libload PUBLIC Includes)
target_include_directories(libload PRIVATE Sources)

target_link_libraries(libload PRIVATE fmt::fmt-header-only)

target_include_directories(libload PRIVATE ${LIBUSB_1_INCLUDE_DIRS})
target_link_libraries(libload PRIVATE ${LIBUSB_1_LIBRARIES})

###############
# External libraries
FetchContent_Declare(cborg
    GIT_REPOSITORY      https://github.com/PelionIoT/cborg.git
)
FetchContent_MakeAvailable(cborg)

add_library(cborg STATIC
    ${cborg_SOURCE_DIR}/source/CborBase.cpp
    ${cborg_SOURCE_DIR}/source/Cbore.cpp
    ${cborg_SOURCE_DIR}/source/Cborg.cpp
)
target_include_directories(cborg PUBLIC ${cborg_SOURCE_DIR})

target_link_libraries(libload PRIVATE cborg)
