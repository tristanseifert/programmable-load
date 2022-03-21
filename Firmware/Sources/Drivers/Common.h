/**
 * @file
 *
 * @brief Driver implementation common support code
 */
#ifndef DRIVERS_COMMON_H
#define DRIVERS_COMMON_H

#include <stddef.h>
#include <stdint.h>

namespace Drivers {
/**
 * @brief Notification bit assignments
 *
 * Defines the usage of the 32 notification bits available in the driver-specific array.
 */
enum NotifyBits: uint32_t {
    /**
     * @brief I²C Master
     */
    I2CMaster                           = (1 << 0),
};
}

#endif
