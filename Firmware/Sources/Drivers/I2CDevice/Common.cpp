#include "Common.h"

#include "Drivers/I2CBus.h"
#include "Log/Logger.h"

using namespace Drivers::I2CDevice;

/**
 * @brief Write a single register
 *
 * Write a byte to a single register
 *
 * @param bus I²C bus the device is connected to
 * @param deviceAddress Address of I²C device
 * @param reg Register to write
 * @param value 8-bit value to write to register
 *
 * @return 0 on success, or an error code
 */
int Common::WriteRegister(I2CBus *bus, const uint8_t deviceAddress, const uint8_t reg,
        const uint8_t value) {
    etl::array<uint8_t, 2> request{
        reg, value,
    };

    etl::array<I2CBus::Transaction, 1> txns{{
        {
            .address = deviceAddress,
            .read = 0,
            .length = 2,
            .data = request
        },
    }};

    return bus->perform(txns);
}

/**
 * @brief Read a single register
 *
 * Read a byte from a device register
 *
 * @param bus I²C bus the device is connected to
 * @param deviceAddress Address of I²C device
 * @param reg Register to read
 * @param outValue Variable to receive the read data; not modified if the transaction fails.
 *
 * @return 0 on success, or an error code
 */
int Common::ReadRegister(I2CBus *bus, const uint8_t deviceAddress, const uint8_t reg,
        uint8_t &outValue) {
    int err;

    etl::array<uint8_t, 1> request{
        reg
    };
    etl::array<uint8_t, 1> reply;

    etl::array<I2CBus::Transaction, 2> txns{{
        // write address
        {
            .address = deviceAddress,
            .read = 0,
            .length = request.size(),
            .data = request
        },
        // read two bytes of register data
        {
            .address = deviceAddress,
            .read = 1,
            .continuation = 1,
            .length = reply.size(),
            .data = reply
        },
    }};

    err = bus->perform(txns);

    if(!err) {
        outValue = reply[0];
    }

    return err;
}

