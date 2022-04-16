/**
 * @file
 *
 * @brief Exported methods
 *
 * Provide all the methods exported by the library.
 */
#include "LibLoad.h"

#include "DeviceTransport.h"
#include "Usb.h"

#include <memory>
#include <stdexcept>

using namespace LibLoad;

/**
 * @brief Device info wrapper
 *
 * This contains information about a connected device, including the transport used to
 * communicate with it.
 */
struct LibLoad::Device {
    /// How is the device connected?
    DeviceInfo::ConnectionMethod method;
    /// serial number of device
    std::string serial;

    /// Transport used to communicate with device
    std::shared_ptr<Internal::DeviceTransport> transport;

    Device(const DeviceInfo &info, std::shared_ptr<Internal::DeviceTransport> &transport) :
        method(info.method), serial(info.serial), transport(std::move(transport)) {}
    Device(const DeviceInfo::ConnectionMethod method, const std::string_view &serial,
            std::shared_ptr<Internal::DeviceTransport> &transport) : method(method),
            serial(serial), transport(std::move(transport)) {}
};


/**
 * @brief Initialize the library
 */
void LibLoad::Init() {
    Internal::Usb::Init();
}

/**
 * @brief Shut down the library
 *
 * Any device connections that are still open will become invalid after this call, and continuing
 * to access them will cause undefined behavior.
 */
void LibLoad::DeInit() {
    Internal::Usb::DeInit();
}



/**
 * @brief Enumerate all connected loads
 *
 * @param callback Function to invoke for each device; it may return `false` to end enumeration
 *
 * @remark Any pointers in the provided device method are not guaranteed to be valid once the
 *         callback returns. Make copies of any strings or other data if you want to keep it around
 *         longer.
 */
void LibLoad::EnumerateDevices(const std::function<bool(const DeviceInfo &)> &callback) {
    Internal::Usb::The()->getDevices([&](const auto &device) -> bool {
        DeviceInfo deviceInfo{
            .method = DeviceInfo::ConnectionMethod::Usb,
            .serial = device.serial
        };
        callback(deviceInfo);

        return true;
    });
}



/**
 * @brief Connect to a device
 *
 * Opens a connection to a device, identified by its information structure.
 *
 * @param info Device information structure
 * @return Device structure (used for later calls) if successful
 */
Device *LibLoad::Connect(const DeviceInfo &info) {
    std::shared_ptr<Internal::DeviceTransport> transport;

    switch(info.method) {
        case DeviceInfo::ConnectionMethod::Usb:
            transport = Internal::Usb::The()->connectBySerial(info.serial);
            break;

        // this should not happen
        default:
            throw std::invalid_argument("invalid transport type");
    }

    // if we failed to allocate a transport, bail
    if(!transport) {
        return nullptr;
    }

    // prepare the device wrapper
    auto dev = new Device(info, transport);

    return dev;
}

/**
 * @brief Connect to a device by serial number
 *
 * Find a device, with the given serial number, and open a connection to it.
 *
 * @param serial Device serial number
 * @return Device structure if successful
 *
 * @remark This only is supported for locally attached (USB) devices
 */
Device *LibLoad::Connect(const std::string_view &serial) {
    auto transport = Internal::Usb::The()->connectBySerial(serial);
    if(!transport) {
        return nullptr;
    }

    auto dev = new Device(DeviceInfo::ConnectionMethod::Usb, serial, transport);
    return dev;
}

/**
 * @brief Disconnect from a device
 *
 * The communications transport is closed, and all resources associated with it are deallocated.
 */
void LibLoad::Disconnect(Device *device) {
    delete device;
}
