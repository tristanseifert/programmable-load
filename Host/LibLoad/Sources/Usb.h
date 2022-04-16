#ifndef USB_H
#define USB_H

#include <stdint.h>

#include <functional>
#include <string>

struct libusb_context;
struct libusb_device;
struct libusb_device_descriptor;

namespace LibLoad::Internal {
/**
 * @brief USB device interface
 *
 * Encapsulates all the libusb shenanigans required to communicate with the USB device.
 */
class Usb {
    public:
        /**
         * @brief Represents a device connected to USB
         */
        struct Device {
            /// USB vendor id
            uint16_t usbVid;
            /// USB product id
            uint16_t usbPid;
            /// USB device address
            uint8_t usbAddress;

            /// Bus on which the device is
            uint8_t bus;
            /// Device number on bus
            uint8_t device;

            /// Device serial number
            std::string serial;
        };

        using DeviceFoundCallback = std::function<bool(const Device&)>;

    public:
        static void Init();
        static void DeInit();

        /// Get shared USB communications instance
        static inline Usb *The() {
            return gShared;
        }

        void getDevices(const DeviceFoundCallback &callback);

    private:
        Usb();
        ~Usb();

        bool probeDevice(const DeviceFoundCallback &, const libusb_device_descriptor &,
                libusb_device *);

    private:
        /// LibUSB context
        libusb_context *usbCtx{nullptr};

    public:
        /// USB vendor id
        constexpr static const uint16_t kUsbVid{0x1209};
        /// USB device/product id
        constexpr static const uint16_t kUsbPid{0x0009};

        static Usb *gShared;
};
}

#endif
