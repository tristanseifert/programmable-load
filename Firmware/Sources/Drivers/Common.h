/**
 * @file
 *
 * @brief Driver implementation common support code
 */
#ifndef DRIVERS_COMMON_H
#define DRIVERS_COMMON_H

#include <stddef.h>
#include <stdint.h>

#include "stm32mp1xx_hal_rcc.h"

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

    /**
     * @brief DMA controller
     */
    DmaController                       = (1 << 1),
};

/**
 * @brief Get the clock frequency of a given APB bus
 *
 * @param bus APB bus number (domain [1,3])
 *
 * @return Frequency, in Hz, the bus is running at. Truncated to the nearest integer.
 */
static constexpr inline uint32_t GetApbClock(const uint8_t bus) {
    uint32_t divisor{0};

    if(bus == 1) {
        switch(__HAL_RCC_GET_APB1_DIV()) {
            case RCC_APB1_DIV1:
                divisor = 1;
                break;
            case RCC_APB1_DIV2:
                divisor = 2;
                break;
            case RCC_APB1_DIV4:
                divisor = 4;
                break;
            case RCC_APB1_DIV8:
                divisor = 8;
                break;
            case RCC_APB1_DIV16:
                divisor = 16;
                break;
        }
    } else if(bus == 2) {
        switch(__HAL_RCC_GET_APB2_DIV()) {
            case RCC_APB2_DIV1:
                divisor = 1;
                break;
            case RCC_APB2_DIV2:
                divisor = 2;
                break;
            case RCC_APB2_DIV4:
                divisor = 4;
                break;
            case RCC_APB2_DIV8:
                divisor = 8;
                break;
            case RCC_APB2_DIV16:
                divisor = 16;
                break;
        }
    } else if(bus == 3) {
        switch(__HAL_RCC_GET_APB3_DIV()) {
            case RCC_APB3_DIV1:
                divisor = 1;
                break;
            case RCC_APB3_DIV2:
                divisor = 2;
                break;
            case RCC_APB3_DIV4:
                divisor = 4;
                break;
            case RCC_APB3_DIV8:
                divisor = 8;
                break;
            case RCC_APB3_DIV16:
                divisor = 16;
                break;
        }
    } else {
        static_assert("invalid apb bus number!");
    }

    return SystemCoreClock / divisor;
}
}

#endif
