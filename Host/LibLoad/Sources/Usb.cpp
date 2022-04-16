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
 *
 * @TODO: Fix this so the device list is always deallocated, even if the probe function throws
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
            libusb_free_device_list(list, 1);
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
 * @brief Find a device, by its serial number, and open a transport to it
 *
 * This is in function similar to the enumeration method, in that it finds all devices with the
 * matching pid/vid values; then compares the serial numbers.
 *
 * @param serial Serial number of the desired device
 *
 * @return Initialized device transport
 *
 * @remark If more than one device has the same serial, the first (in enumeration order) will win
 */
std::shared_ptr<DeviceTransport> Usb::connectBySerial(const std::string_view &serial) {
    int err;
    libusb_device **list;

    std::shared_ptr<Transport> transport;

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
            libusb_free_device_list(list, 1);
            throw LibUsbError(err, "libusb_get_device_descriptor");
        }

        // ensure the vid/pid match
        if(desc.idVendor != kUsbVid || desc.idProduct != kUsbPid) {
            continue;
        }

        // open the device
        libusb_device_handle *handle{nullptr};
        err = libusb_open(device, &handle);
        if(err) {
            libusb_free_device_list(list, 1);
            throw LibUsbError(err, "libusb_open");
        }

        // compare serial number
        const auto readSerial = ReadStringDescriptor(handle, desc.iSerialNumber);
        if(readSerial != serial) {
            libusb_close(handle);
            continue;
        }

        // initialize a transport
        transport = std::make_shared<Transport>(handle);
        goto beach;
    }

beach:;
    libusb_free_device_list(list, 1);
    return transport;
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



/**
 * @brief Initialize USB device transport
 *
 * This will claim the vendor interface so that we can communicate with it.
 *
 * @remark This assumes the device has been configured before the transport is constructed; this
 *         should always be true as the current firmware exposes only one configuration.
 */
Usb::Transport::Transport(libusb_device_handle *handle) : device(handle) {
    int err;
    auto device = libusb_get_device(handle);

    struct libusb_config_descriptor *cfgDescriptor{nullptr};

    // claim the interface, so that we can perform IO against it
    err = libusb_claim_interface(handle, static_cast<int>(Interface::Vendor));
    if(err) {
        throw LibUsbError(err, "libusb_claim_interface");
    }

    // then get the endpoints for the interface
    err = libusb_get_active_config_descriptor(device, &cfgDescriptor);
    if(err) {
        throw LibUsbError(err, "libusb_get_active_config_descriptor");
    }

    if(cfgDescriptor->bNumInterfaces < static_cast<uint8_t>(Interface::MaxNumInterfaces)) {
        throw std::runtime_error("Insufficient number of interfaces");
    }

    // select the first interface (out of the alternate settings)
    const auto &vendorIntf = cfgDescriptor->interface[static_cast<uint8_t>(Interface::Vendor)]
        .altsetting[0];
    if(vendorIntf.bNumEndpoints < 2) {
        throw std::runtime_error("Insufficient number of endpoints");
    }

    for(size_t i = 0; i < vendorIntf.bNumEndpoints; i++) {
        const auto &ep = vendorIntf.endpoint[i];

        // IN endpoint?
        if(ep.bEndpointAddress & (1 << 7)) {
            this->epIn = ep.bEndpointAddress;
        }
        // OUT endpoint
        else {
            this->epOut = ep.bEndpointAddress;
        }
    }

    // clean up
    libusb_free_config_descriptor(cfgDescriptor);
}

/**
 * @brief Tear down USB device transport
 *
 * We'll release the vendor interface here and then close the device.
 */
Usb::Transport::~Transport() noexcept(false) {
    int err;

    err = libusb_release_interface(this->device, static_cast<int>(Interface::Vendor));
    if(err) {
        throw LibUsbError(err, "libusb_release_interface");
    }

    libusb_close(this->device);
}

/**
 * @brief Write data to USB device
 *
 * Transmits the specified data to the device. We rely on libusb to split the data into more than
 * one USB packet if needed.
 *
 * @throw std::invalid_argument In case parameters are invalid
 * @throw LibUsbError Underlying IO error
 */
void Usb::Transport::write(const uint8_t type, const std::span<uint8_t> payload,
        std::optional<std::chrono::milliseconds> timeout) {
    int err, transferred{0};

    // validate inputs
    if(payload.empty()) {
        throw std::invalid_argument("invalid payload");
    }
    else if(payload.size() > kMaxPacketSize) {
        throw std::invalid_argument("payload too large");
    }

    // combine the packet header and payload in the transmit buffer
    PacketHeader hdr(type, payload.size());
    const auto bytesRequired = sizeof(hdr) + payload.size();

    this->buffer.resize(bytesRequired, 0);

    memcpy(this->buffer.data(), &hdr, sizeof(hdr));

    if(!payload.empty()) { // XXX: do we need to support empty payloads?
        std::copy(this->buffer.begin() + sizeof(hdr), this->buffer.end(), payload.begin());
    }

    // transmit the whole thing
    err = libusb_bulk_transfer(this->device, this->epOut, buffer.data(), bytesRequired,
            &transferred, timeout ? (*timeout).count() : 0);
    if(err) {
        throw LibUsbError(err, "libusb_bulk_transfer (write)");
    }

    if(transferred != sizeof(hdr)) {
        throw std::runtime_error(fmt::format("partial transfer: {}, expected {}", transferred,
                    bytesRequired));
    }
}

