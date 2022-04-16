#ifndef USB_H
#define USB_H

#include "DeviceTransport.h"

#include <arpa/inet.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

struct libusb_context;
struct libusb_device;
struct libusb_device_descriptor;
struct libusb_device_handle;

namespace LibLoad::Internal {
/**
 * @brief USB device interface
 *
 * Encapsulates all the libusb shenanigans required to communicate with the USB device.
 */
class Usb {
    private:
        /**
         * @brief Interface indices
         *
         * These are the indices of interfaces on the USB device.
         */
        enum class Interface: uint8_t {
            /// Vendor specific interface
            Vendor                      = 0,

            MaxNumInterfaces,
        };

        /**
         * @brief USB device transport
         *
         * Handles sending/receiving messages with USB devices by means of the blocking libusb
         * interface.
         */
        class Transport: public DeviceTransport {
            private:
                /**
                 * @brief USB packet header
                 *
                 * This is a small 4 byte header prepended to all packets sent over the USB
                 * interface to the device.
                 *
                 * @remark All multibyte values are sent in network (big endian) byte order.
                 */
                struct PacketHeader {
                    /**
                     * @brief Message type
                     *
                     * Defines the format of the content of the message. Each type is associated
                     * with a specific type of handler.
                     */
                    uint8_t type;

                    /**
                     * @brief Message tag
                     *
                     * The tag value is used to match up a request to corresponding response from
                     * the device.
                     */
                    uint8_t tag;

                    /**
                     * @brief Payload length (bytes)
                     *
                     * If nonzero, this is the number of payload data bytes that follow immediately
                     * after the packet header.
                     */
                    uint16_t payloadLength;

                    PacketHeader() = default;
                    PacketHeader(const uint8_t type, const size_t length) : type(type) {
                        if(length > kMaxPacketSize) {
                            throw std::invalid_argument("payload too large");
                        }

                        this->payloadLength = htons(length & 0x3ff);
                    }

                    /// Get the payload length
                    constexpr inline size_t getPayloadLength() const {
                        return ntohs(this->payloadLength);
                    }
                } __attribute__((packed));

                /// Maximum packet size, in bytes
                constexpr static const size_t kMaxPacketSize{512};

            public:
                Transport(libusb_device_handle *device);
                ~Transport() noexcept(false);

                void write(const uint8_t type, const std::span<uint8_t> payload,
                        std::optional<std::chrono::milliseconds> timeout) override;

            private:
                /// packet send buffer
                std::vector<uint8_t> buffer;

                /// Underlying USB device to communicate on
                libusb_device_handle *device;

                /// endpoint used to send data TO device
                uint8_t epOut{0};
                /// endpoint used to receive data FROM device
                uint8_t epIn{0};
        };

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

        std::shared_ptr<DeviceTransport> connectBySerial(const std::string_view &serial);

    private:
        Usb();
        ~Usb();

        bool probeDevice(const DeviceFoundCallback &, const libusb_device_descriptor &,
                libusb_device *);

        static std::string ReadStringDescriptor(libusb_device_handle *device,
                const uint8_t index);

    private:
        /// LibUSB context
        libusb_context *usbCtx{nullptr};

    public:
        /// USB vendor id
        constexpr static const uint16_t kUsbVid{0x1209};
        /// USB device/product id
        constexpr static const uint16_t kUsbPid{0x0009};

        /**
         * @brief String descriptor language id
         *
         * This is the language id for string descriptors to look up. The current device firmware
         * completely ignores this value, but we'll specify this default value rather than zero;
         * technically we probably should read descriptor 0 to figure out what the supported
         * languages are.
         */
        constexpr static const uint16_t kLanguageId{0x0409};

        static Usb *gShared;
};
}

#endif
