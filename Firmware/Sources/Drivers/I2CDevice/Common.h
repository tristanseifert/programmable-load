#ifndef DRIVERS_I2CDEVICE_COMMON_H
#define DRIVERS_I2CDEVICE_COMMON_H

#include <stddef.h>
#include <stdint.h>

namespace Drivers {
class I2CBus;
}

namespace Drivers::I2CDevice {
/**
 * @brief Common I²C device helpers
 *
 * Implements some common helpers to interface to I²C peripherals.
 */
class Common {
    public:
        static int WriteRegister(I2CBus *bus, const uint8_t deviceAddress, const uint8_t reg,
                const uint8_t value);
        static int ReadRegister(I2CBus *bus, const uint8_t deviceAddress, const uint8_t reg,
                uint8_t &outValue);
};
}

#endif
