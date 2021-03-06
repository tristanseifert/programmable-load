#ifndef DEVICETRANSPORT_H
#define DEVICETRANSPORT_H

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace LibLoad::Internal {
/**
 * @brief Abstract interface for a connection to a device
 *
 * Concrete subclasses of this abstract base class are exposed by each of the different
 * transports with which a device can be connected. These device connection subclasses handle the
 * (de)serialization of messages, and transacting with the actual device.
 */
class DeviceTransport {
    public:
        virtual ~DeviceTransport() noexcept(false) = default;

        /**
         * @brief Transmit a packet to the device
         *
         * Send a packet of binary data to the device without waiting for a response.
         *
         * Depending on the underlying transport, the device connection can add one or more bytes
         * of additional header or footer data between the payload. This auxiliary data is
         * required to carry the specified message type.
         *
         * @param type Message type
         * @param payload Data to send as part of message payload
         * @param timeout How long we should wait before giving up
         */
        virtual void write(const uint8_t type, const std::span<const uint8_t> payload,
                std::optional<std::chrono::milliseconds> timeout = std::nullopt) = 0;

        /**
         * @brief Receive a packet from the device
         *
         * Receive data from the device. It's valid for the device to return less than the total
         * size of the buffer.
         *
         * @param buffer Buffer to receive the data
         * @param length Number of bytes to receive, or 0 to receive the full buffer
         * @param timeout How long we should wait before giving up
         *
         * @return Total number of bytes received
         */
        virtual size_t read(std::span<uint8_t> buffer, const size_t length = 0,
                std::optional<std::chrono::milliseconds> timeout = std::nullopt) = 0;
};
}

#endif
