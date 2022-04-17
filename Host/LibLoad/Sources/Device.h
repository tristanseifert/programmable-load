#ifndef DEVICE_H
#define DEVICE_H

#include "LibLoad.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>

#include <nlohmann/json.hpp>
#include <cborg/Cbor.h>

namespace LibLoad::Internal {
class DeviceTransport;

/**
 * @brief Programmable load interface
 *
 * This wraps the underlying device transport, and provides high level methods for accessing the
 * actual device.
 */
class DeviceImpl: public LibLoad::Device {
    public:
        /// Maximum size of a received packet
        constexpr static const size_t kMaxPacketSize{1024};

    private:
        /**
         * @brief Message endpoint types
         *
         * These are all currently defined message endpoints in device firmware.
         */
        enum class Endpoint: uint8_t {
            PropertyRequest             = 0x01,
        };

    public:
        DeviceImpl(const DeviceInfo &info, std::shared_ptr<Internal::DeviceTransport> &transport) :
            method(info.method), serial(info.serial), transport(std::move(transport)) {}
        DeviceImpl(const DeviceInfo::ConnectionMethod method, const std::string_view &serial,
                std::shared_ptr<Internal::DeviceTransport> &transport) : method(method),
                serial(serial), transport(std::move(transport)) {}

        /// Return the device's serial number
        const std::string &getSerialNumber() const override {
            return this->serial;
        }
        /// Return the device's connection method
        DeviceInfo::ConnectionMethod getConnectionMethod() const override {
            return this->method;
        }

        /**
         * @brief Read a single property as a string
         */
        bool propertyRead(const Property key, std::string &outValue) override {
            nlohmann::json value;
            this->propertyGet(key, value);

            if(!value.is_null()) {
                value.get_to(outValue);
                return true;
            }

            return false;
        }

    private:
        bool propertyGet(const Property key, nlohmann::json &outValue);

        void writeCborMessage(const Endpoint type, const nlohmann::json &payload);
        void readCborMessage(Cborg &message);

    private:
        /**
         * @brief Device lock
         *
         * During all IO transactions, this lock is taken to ensure no other threads can mess with
         * the device state.
         */
        std::mutex lock;

        /// How is the device connected?
        DeviceInfo::ConnectionMethod method;
        /// serial number of device
        std::string serial;

        /**
         * @brief Device transport
         *
         * This is used to communicate with the device.
         */
        std::shared_ptr<Internal::DeviceTransport> transport;

        /// Receive message buffer
        std::array<uint8_t, kMaxPacketSize> buffer;
};
}

#endif
