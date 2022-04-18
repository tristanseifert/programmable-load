#ifndef DEVICE_H
#define DEVICE_H

#include "LibLoad.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>

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
         * @brief Read a single property as an unsigned integer
         */
        bool propertyRead(const Property key, unsigned int &outValue) override {
            uint32_t temp{0};
            auto val = this->propertyGet(key);
            if(val.getUnsigned(&temp)) {
                outValue = temp;
                return true;
            }
            return false;
        }

        /**
         * @brief Read a single property as a signed integer
         */
        bool propertyRead(const Property key, int &outValue) override {
            int32_t temp{0};
            auto val = this->propertyGet(key);
            if(val.getNegative(&temp)) {
                outValue = temp;
                return true;
            }
            return false;
        }

        /**
         * @brief Read a single property as a string
         */
        bool propertyRead(const Property key, std::string &outValue) override {
            auto val = this->propertyGet(key);
            return val.getString(outValue);
        }

    private:
        Cborg propertyGet(const Property key);

        void writeCborMessage(const Endpoint type, const std::function<Cbore(Cbore)> &encoder);
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
        std::array<uint8_t, kMaxPacketSize> rxBuffer;
        /// Send message (payload) buffer (used for CBOR serialization)
        std::array<uint8_t, kMaxPacketSize> txBuffer;
};
}

#endif
