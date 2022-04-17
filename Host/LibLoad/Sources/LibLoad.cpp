/**
 * @file
 *
 * @brief Exported methods
 *
 * Provide all the methods exported by the library.
 */
#include "LibLoad.h"
#include "Device.h"

#include "DeviceTransport.h"
#include "Usb.h"

#include <memory>
#include <stdexcept>

using namespace LibLoad;



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
    auto dev = new Internal::DeviceImpl(info, transport);

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

    auto dev = new Internal::DeviceImpl(DeviceInfo::ConnectionMethod::Usb, serial, transport);
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
