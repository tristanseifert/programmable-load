#ifndef DRIVERS_I2CDEVICE_AT24CS32_H
#define DRIVERS_I2CDEVICE_AT24CS32_H

#include <stddef.h>
#include <stdint.h>

#include <etl/span.h>

namespace Drivers {
class I2CBus;
}

namespace Drivers::I2CDevice {
/**
 * @brief I²C EEPROM with serial number
 *
 * The AT24CS32 is a 32Kbit (4K x 8) EEPROM, and also features an embedded, unique 128-bit serial
 * number. This serial is preprogrammed from the factory and guaranteed unique among all devices in
 * the series.
 *
 * @remark Write accesses are rather slow; we always wait the maximum 5ms write delay, rather than
 *         polling for completion, for simplicity.
 */
class AT24CS32 {
    public:
        /**
         * @brief Error codes emitted by the device
         */
        enum Errors: int {
            /**
             * @brief The specified buffer is invalid
             *
             * The buffer may be too small (or too large) for the given request.
             */
            InvalidBuffer               = -5200,
        };

        /**
         * @brief User memory area size
         *
         * Size of the EEPROM array that's writable by the user, in bytes.
         */
        constexpr static const size_t kDeviceSize{0x800};

        /**
         * @brief Size of a single page
         */
        constexpr static const size_t kPageSize{0x20};

        /**
         * @brief Default device address
         *
         * This is the address of the device with all address inputs tied to 0. Note that the
         * serial number component of the device has a different bus address, which is found by
         * adding kSerialAddressOffset to the base address.
         */
        constexpr static const uint8_t kDefaultAddress{0b1010000};

        /**
         * @brief Address offset from data array to serial number array
         *
         * Add this value to the base address to get the address of the serial number region.
         */
        constexpr static const uint8_t kSerialAddressOffset{0b0001000};

    public:
        /**
         * @brief Initialize the EEPROM
         *
         * Sets up the driver for the EEPROM with its bus and bus address.
         */
        AT24CS32(Drivers::I2CBus *bus, const uint8_t address = kDefaultAddress) : bus(bus),
            deviceAddress(address) { }

        /**
         * @brief Read from EEPROM array
         *
         * Performs a read against the EEPROM's user data area, starting a the given address.
         *
         * @param address Starting word address in the memory
         * @param buffer Buffer to receive the read data
         *
         * @return 0 on success, or a negative error code.
         *
         * @remark The address will increment by one after each byte written, and wraps around
         *         after the full memory array size.
         */
        int readData(const uint16_t address, etl::span<uint8_t> buffer) {
            return Read(this->bus, this->deviceAddress, address, buffer);
        }

        /**
         * @brief Write to the EEPROM array
         *
         * Writes data to the EEPROM. Internally, this attempts to use page writes for maximum
         * efficiency.
         *
         * @param address Starting address for the write
         * @param data Data to write to the memory array
         *
         * @return 0 on success, or a negative error code.
         *
         * @remark There is no way to check whether the device is write protected, other than to
         *         read back the value of the memory array after attempting a write.
         */
        int writeData(uint16_t address, etl::span<const uint8_t> data) {
            int err;

            size_t offset{0};

            // write partial start page
            if(address & (kPageSize - 1)) {
                const auto bytes = kPageSize - (address & (kPageSize - 1));
                err = PageWrite(this->bus, this->deviceAddress, address, data, bytes);
                if(err) {
                    return err;
                }

                offset += bytes;
            }

            // write full pages
            const auto fullPages = (data.size() - offset) / kPageSize;
            if(fullPages) {
                for(size_t page = 0; page < fullPages; page++) {
                    etl::span<const uint8_t> subspan(data.begin() + offset, kPageSize);

                    err = PageWrite(this->bus, this->deviceAddress, address + offset, subspan, kPageSize);
                    if(err) {
                        return err;
                    }
                    offset += kPageSize;
                }
            }

            // write any remaining partial page
            if(offset < data.size()) {
                etl::span<const uint8_t> subspan(data.begin() + offset, data.size() - offset);
                err = PageWrite(this->bus, this->deviceAddress, address + offset, subspan,
                        subspan.size());
                if(err) {
                    return err;
                }
            }

            // if we get here, assume success
            return 0;
        }

        /**
         * @brief Read serial number
         *
         * Reads the entire 128 bit serial number from the device.
         *
         * @param buffer Buffer to receive the serial number, must be at least 16 bytes
         *
         * @return 0 on success, or a negative error code.
         */
        int readSerial(etl::span<uint8_t, 16> buffer) {
            return Read(this->bus, this->deviceAddress + kSerialAddressOffset, 0x800, buffer);
        }

    private:
        static int Read(Drivers::I2CBus *bus, const uint8_t deviceAddress, const uint16_t start,
                etl::span<uint8_t> buffer);
        static int PageWrite(Drivers::I2CBus *bus, const uint8_t deviceAddress,
                const uint16_t start, etl::span<const uint8_t> buffer, const size_t numBytes = 0);

    private:
        /// Parent bus
        Drivers::I2CBus *bus;
        /// Device address (for EEPROM array)
        uint8_t deviceAddress;
};
}

#endif
