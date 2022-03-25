#include "AT24CS32.h"

#include "Drivers/I2CBus.h"
#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include <etl/array.h>

using namespace Drivers::I2CDevice;

/**
 * @brief Read from the device
 *
 * This writes the two address bytes (in big endian order), then a restart to read the given
 * number of bytes.
 *
 * @param bus I²C bus to perform IO against
 * @param deviceAddress I²C address of the device
 * @param start Starting memory address
 * @param buffer Buffer to receive the read data
 *
 * @return 0 on success, or an error code.
 */
int AT24CS32::Read(Drivers::I2CBus *bus, const uint8_t deviceAddress, const uint16_t start,
        etl::span<uint8_t> buffer) {
    // validate buffer
    if(buffer.empty() || buffer.size() > kDeviceSize) {
        return Errors::InvalidBuffer;
    }

    // set up the request
    etl::array<uint8_t, 2> addressBuf{
        static_cast<uint8_t>((start & 0xFF00) >> 8), static_cast<uint8_t>(start & 0x00FF)
    };

    etl::array<I2CBus::Transaction, 2> txns{{
        // write address
        {
            .address = deviceAddress,
            .read = 0,
            .length = 2,
            .data = addressBuf
        },
        // read data
        {
            .address = deviceAddress,
            .read = 1,
            .continuation = 1,
            .length = static_cast<uint16_t>(buffer.size()),
            .data = buffer,
        },
    }};

    return bus->perform(txns);
}

/**
 * @brief Perform a page write
 *
 * Writes a single page (up to 32 bytes) to the device.
 *
 * @param bus I²C bus to perform IO against
 * @param deviceAddress I²C address of the device
 * @param start Starting memory address for the write
 * @param buffer Buffer containing data to write
 * @param numBytes Number of bytes to write, or 0 for entire buffer
 *
 * @return 0 on success, or an error code
 *
 * @remark A single page write may write at most 32 bytes, if the starting address is 32-byte
 *         aligned. Otherwise, the write length is reduced to the remainder of the 32-byte chunk,
 *         as wraparound in page writes is probably unintended.
 */
int AT24CS32::PageWrite(Drivers::I2CBus *bus, const uint8_t deviceAddress,
        const uint16_t start, etl::span<uint8_t> buffer, const size_t numBytes) {
    int err;

    // validate buffer
    if(buffer.empty() || buffer.size() > kPageSize) {
        return Errors::InvalidBuffer;
    }

    const auto pageOff = start & (kPageSize - 1);
    if(buffer.size() > (kPageSize - pageOff)) {
        return Errors::InvalidBuffer;
    }
    if(numBytes > buffer.size()) {
        return Errors::InvalidBuffer;
    }

    // do the write
    etl::array<uint8_t, 2> addressBuf{
        static_cast<uint8_t>((start & 0xFF00) >> 8), static_cast<uint8_t>(start & 0x00FF)
    };

    etl::array<I2CBus::Transaction, 2> txns{{
        // write address
        {
            .address = deviceAddress,
            .read = 0,
            .length = 2,
            .data = addressBuf
        },
        // write data
        {
            .address = deviceAddress,
            .read = 0,
            .continuation = 1,
            .length = static_cast<uint16_t>(numBytes ? numBytes : buffer.size()),
            .data = buffer,
        },
    }};

    err = bus->perform(txns);

    if(!err) {
        // insert write delay
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    return err;
}

