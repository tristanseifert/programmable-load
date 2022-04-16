/**
 * @file
 *
 * @brief LibLoad main header
 */
#ifndef LIBLOAD_H
#define LIBLOAD_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

namespace LibLoad {
/**
 * @brief Information about a device
 *
 * This structure represents information about a single device, which may be connected to.
 */
struct DeviceInfo {
    enum class ConnectionMethod: uint8_t {
        Unknown                         = 0,
        Usb                             = 1,
        Network                         = 2,
    };

    /// How is the device connected?
    ConnectionMethod method;
    /// Device serial number
    std::string serial;
};

struct Device;

void Init();
void DeInit();

void EnumerateDevices(const std::function<bool(const DeviceInfo &info)> &callback);

Device *Connect(const DeviceInfo &info);
Device *Connect(const std::string_view &serial);

void Disconnect(Device *device);
}

#endif
