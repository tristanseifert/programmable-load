/**
 * @file
 *
 * @brief LibLoad main header
 */
#ifndef LIBLOAD_H
#define LIBLOAD_H

#include <stddef.h>
#include <stdint.h>

#include <functional>

namespace LibLoad {
/**
 * @brief Information about a device
 *
 * This structure represents information about a single device, which may be connected to.
 */
struct Device {
    enum class ConnectionMethod: uint8_t {
        Unknown                         = 0,
        Usb                             = 1,
        Network                         = 2,
    };

    /// How is the device connected?
    ConnectionMethod method;
    /// Device serial number
    const char *serial;
};

void Init();
void DeInit();

void EnumerateDevices(const std::function<bool(const Device &deviceInfo)> &callback);
}

#endif
