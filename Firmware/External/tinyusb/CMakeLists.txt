####################################################################################################
# TinyUSB: USB stack
#
# This currently only supports a subset of the microcontrollers the actual stack supports, and also
# is limited to just working in device mode.
####################################################################################################
# get the code from git repo
FetchContent_Declare(tinyusb
    GIT_REPOSITORY https://github.com/hathach/tinyusb.git
    GIT_TAG 0.13.0
)
FetchContent_MakeAvailable(tinyusb)

# Create a static library for the base TinyUSB stack
add_library(tinyusb STATIC
    ${tinyusb_SOURCE_DIR}/src/tusb.c
    ${tinyusb_SOURCE_DIR}/src/common/tusb_fifo.c
    ${tinyusb_SOURCE_DIR}/src/device/usbd.c
    ${tinyusb_SOURCE_DIR}/src/device/usbd_control.c
    ${tinyusb_SOURCE_DIR}/src/class/audio/audio_device.c
    ${tinyusb_SOURCE_DIR}/src/class/cdc/cdc_device.c
    ${tinyusb_SOURCE_DIR}/src/class/dfu/dfu_device.c
    ${tinyusb_SOURCE_DIR}/src/class/dfu/dfu_rt_device.c
    ${tinyusb_SOURCE_DIR}/src/class/hid/hid_device.c
    ${tinyusb_SOURCE_DIR}/src/class/midi/midi_device.c
    ${tinyusb_SOURCE_DIR}/src/class/msc/msc_device.c
    ${tinyusb_SOURCE_DIR}/src/class/net/ecm_rndis_device.c
    ${tinyusb_SOURCE_DIR}/src/class/net/ncm_device.c
    ${tinyusb_SOURCE_DIR}/src/class/usbtmc/usbtmc_device.c
    ${tinyusb_SOURCE_DIR}/src/class/video/video_device.c
    ${tinyusb_SOURCE_DIR}/src/class/vendor/vendor_device.c
)

target_include_directories(tinyusb PUBLIC ${CMAKE_CURRENT_LIST_DIR}/Includes)
target_include_directories(tinyusb PUBLIC ${tinyusb_SOURCE_DIR}/src)

####################################################################################################
# Platform specific code for Atmel/Microchip SAMD (D5x) chips
add_library(tinyusb-platform-atsamd STATIC
    ${tinyusb_SOURCE_DIR}/src/portable/microchip/samd/dcd_samd.c
)

target_include_directories(tinyusb-platform-atsamd PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/Portable/ATSAMD/Includes)
target_link_libraries(tinyusb-platform-atsamd tinyusb)
