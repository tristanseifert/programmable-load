####################################################################################################
# Programmable load interface library
####################################################################################################
# Find LibUSB
find_package(LibUSB)

# Define the library
add_library(libload STATIC
    Sources/LibLoad.cpp
    Sources/Usb.cpp
)
target_include_directories(libload PUBLIC Includes)
target_include_directories(libload PRIVATE Sources)

target_link_libraries(libload PRIVATE nlohmann_json::nlohmann_json fmt::fmt-header-only)

target_include_directories(libload PRIVATE ${LIBUSB_1_INCLUDE_DIRS})
target_link_libraries(libload PRIVATE ${LIBUSB_1_LIBRARIES})
