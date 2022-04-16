#include "Usb.h"
#include "LibUsbError.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <codecvt>
#include <iostream>
#include <locale>

#include <libusb.h>

using namespace LibLoad::Internal;

Usb *Usb::gShared{nullptr};

/**
 * @brief Initialize the shared USB communications interface
 */
void Usb::Init() {
    assert(!gShared);

    auto usb = new Usb;
    gShared = usb;
}

/**
 * @brief Shut down the shared USB communications interface
 */
void Usb::DeInit() {
    assert(gShared);

    delete gShared;
    gShared = nullptr;
}



/**
 * @brief Initialize USB communication interface
 *
 * This configures the libusb context.
 */
Usb::Usb() {
    int err;

    err = libusb_init(&this->usbCtx);
    if(err) {
        throw LibUsbError(err, "libusb_init");
    }
}

/**
 * @brief Tear down the USB communications interface
 */
Usb::~Usb() {
    // shut down libusb
    libusb_exit(this->usbCtx);
}



/**
 * @brief Enumerate all devices on the bus
 */
void Usb::getDevices(const DeviceFoundCallback &callback) {
    int err;
    libusb_device **list;

    // query device list
    const auto cnt = libusb_get_device_list(this->usbCtx, &list);

    if(cnt < 0) {
        throw LibUsbError(cnt, "libusb_get_device_list");
    }

    for(size_t i = 0; i < cnt; i++) {
        // read device descriptor
        struct libusb_device_descriptor desc;
        auto device = list[i];

        err = libusb_get_device_descriptor(device, &desc);
        if(err) {
            throw LibUsbError(err, "libusb_get_device_descriptor");
        }

        // ensure the vid/pid match
        if(desc.idVendor != kUsbVid || desc.idProduct != kUsbPid) {
            continue;
        }

        // get some more information from the device
        if(!this->probeDevice(callback, desc, device)) {
            goto beach;
        }
    }

beach:;
    // clean up device list
    libusb_free_device_list(list, 1);
}

/**
 * @brief Open a device and extract info from it, then invoke device callback
 *
 * Reads the serial number and bus location of a device, then populates a device information
 * structure and invokes the device callback.
 *
 * @param callback Function to invoke with device information
 * @param desc USB device descriptor
 * @param device The libusb device instance
 *
 * @return Whether we should continue enumeration
 */
bool Usb::probeDevice(const DeviceFoundCallback &callback, const libusb_device_descriptor &desc,
        libusb_device *device) {
    int err;
    libusb_device_handle *handle{nullptr};

    // open the device
    err = libusb_open(device, &handle);
    if(err) {
        throw LibUsbError(err, "libusb_open");
    }

    // build an info struct and invoke callback
    const Device dev{
        .usbVid = desc.idVendor,
        .usbPid = desc.idProduct,
        .usbAddress = libusb_get_device_address(device),
        .bus = libusb_get_bus_number(device),
        .device = libusb_get_port_number(device),
        .serial = ReadStringDescriptor(handle, desc.iSerialNumber),
    };

    const auto ret = callback(dev);

    // clean up device
    libusb_close(handle);

    return ret;
}



/**
 * @brief Read a device's string descriptor
 *
 * Read the string descriptor at the given index from the device, then convert it to UTF-8 and
 * return the string.
 *
 * @param device Device to read the string descriptor from
 * @param index String descriptor index to read
 *
 * @return The read string descriptor
 *
 * @throw LibUsbError If an error took place communicating with the device
 */
std::string Usb::ReadStringDescriptor(libusb_device_handle *device, const uint8_t index) {
    int err;
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;

    // read the descriptor
    std::array<char16_t, 256> buffer;
    std::fill(buffer.begin(), buffer.end(), 0);

    err = libusb_get_string_descriptor(device, index, kLanguageId,
            reinterpret_cast<unsigned char *>(buffer.data()), buffer.size());
    if(err < 0) {
        throw LibUsbError(err, "libusb_get_string_descriptor");
    }

    // convert to UTF 8 (skipping the first byte, which is length)
    return conv.to_bytes(buffer.data() + 1);
}
