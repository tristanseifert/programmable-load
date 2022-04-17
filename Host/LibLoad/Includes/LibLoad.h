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

/**
 * @brief Device interface
 *
 * Provides a generic interface to a programmable load device.
 */
struct Device {
    /**
     * @brief Property keys
     *
     * All properties supported by the device.
     */
    enum class Property {
        HwSerial                        = 0x01,
        HwVersion                       = 0x02,
        HwInventory                     = 0x03,
        SwVersion                       = 0x04,
    };

    virtual ~Device() = default;

    /**
     * @brief Get the device's serial number
     */
    virtual const std::string &getSerialNumber() const = 0;
    /**
     * @brief Get how the device is connected
     */
    virtual DeviceInfo::ConnectionMethod getConnectionMethod() const = 0;

    /// Read a property (as a string value)
    virtual bool propertyRead(const Property id, std::string &outValue) = 0;
    /// Read a property (as a signed integer value)
    //virtual bool propertyRead(const Property id, int &outValue) = 0;
    /// Read a property (as an unsigned integer value)
    //virtual bool propertyRead(const Property id, unsigned int &outValue) = 0;
    /// Read a property (as a floating point)
    //virtual bool propertyRead(const Property id, double &outValue) = 0;
};

void Init();
void DeInit();

void EnumerateDevices(const std::function<bool(const DeviceInfo &info)> &callback);

Device *Connect(const DeviceInfo &info);
Device *Connect(const std::string_view &serial);

void Disconnect(Device *device);
}

#endif
