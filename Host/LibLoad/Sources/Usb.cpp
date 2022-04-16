#include "Usb.h"
#include "LibUsbError.h"

#include <algorithm>
#include <array>
#include <cassert>

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
        throw LibUsbError(err, "libusb_device_handle");
    }

    // read serial number
    std::array<char, 32> serialBuf;
    std::fill(serialBuf.begin(), serialBuf.end(), 0);

    err = libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber,
            reinterpret_cast<unsigned char *>(serialBuf.data()), serialBuf.size());
    if(err < 0) {
        libusb_close(handle);

        throw LibUsbError(err, "libusb_device_handle");
    }

    // build an info struct and invoke callback
    const Device dev{
        .usbVid = desc.idVendor,
        .usbPid = desc.idProduct,
        .usbAddress = libusb_get_device_address(device),
        .bus = libusb_get_bus_number(device),
        .device = libusb_get_port_number(device),
        .serial = std::string(serialBuf.data())
    };

    return callback(dev);
}
