/**
 * @file
 *
 * @brief Exported methods
 *
 * Provide all the methods exported by the library.
 */
#include "LibLoad.h"
#include "Usb.h"

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
void LibLoad::EnumerateDevices(const std::function<bool(const Device &)> &callback) {
    Internal::Usb::The()->getDevices([&](const auto &device) -> bool {
        Device deviceInfo{
            .method = Device::ConnectionMethod::Usb,
            .serial = device.serial.c_str()
        };
        callback(deviceInfo);

        return true;
    });
}

