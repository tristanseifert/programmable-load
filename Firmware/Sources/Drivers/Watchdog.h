#ifndef DRIVER_WATCHDOG_H
#define DRIVER_WATCHDOG_H

#include <stddef.h>
#include <stdint.h>

#include "Rtos/Rtos.h"

extern "C" {
void WDT_Handler();
}

namespace Drivers {
/**
 * @brief Watchdog driver
 *
 * Provides support for the system watchdog, in both regular and windowed mode. The early warning
 * interrupt may be enabled, which can be used to notify a chosen task.
 */
class Watchdog {
    friend void ::WDT_Handler();

    public:
        /**
         * @brief Clock divider
         *
         * Settings possible for the watchdog's internal clock dividers. The clock is divided off
         * a 1.024kHz reference, which is derived from the internal ultra low power 32kHz
         * oscillator. This means it's not totally accurate.
         */
        enum class ClockDivider: uint8_t {
            Div8                        = 0x0,
            Div16                       = 0x1,
            Div32                       = 0x2,
            Div64                       = 0x3,
            Div128                      = 0x4,
            Div256                      = 0x5,
            Div512                      = 0x6,
            Div1024                     = 0x7,
            Div2048                     = 0x8,
            Div4096                     = 0x9,
            Div8192                     = 0xA,
            Div16384                    = 0xB,
        };

        /// Watchdog configuration
        struct Config {
            /**
             * @brief Watchdog timeout period
             *
             * Defines the timeout for the watchdog, in terms of watchdog clock cycles. The clock
             * of the watchdog runs at roughly 1kHz.
             */
            ClockDivider timeout;

            /**
             * @brief Secondary timeout
             *
             * Defines the secondary watchdog timeout. This depends on the mode:
             *
             * - Normal mode: Time at which the early warning interrupt is generated
             * - Window mode: Start of the watchdog window opening
             */
            ClockDivider secondary;

            /**
             * @brief Window mode enable
             *
             * When set, the watchdog operates in window mode. In this mode, in addition to needing
             * to be petted _before_ its primary timeout elapses, this must also take place _after_
             * a secondary timeout.
             */
            uint8_t windowMode:1{0};

            /**
             * @brief Early warning interrupt enable
             *
             * When enabled, an early warning interrupt is generated (which in turn will notify
             * the specified task) after the secondary timeout elapses. (In windowed mode, this
             * will coincide with the opening of the window.)
             */
            uint8_t earlyWarningIrq:1{0};

            /**
             * @brief Early warning task
             *
             * When the early warning interrupt is enabled, this is the task that will receive a
             * notification. A specified notification index will have one or more bits set.
             */
            TaskHandle_t notifyTask{nullptr};
            /**
             * @brief Task notification index
             *
             * The index to notify the specified task under.
             */
            size_t notifyIndex{0};
            /**
             * @brief Task notification bits
             *
             * Notification bits to set on the specified task when the early warning interrupt is
             * fired.
             */
            uintptr_t notifyBits{0};
        };

    public:
        static void Configure(const Config &conf);

        static void Enable();
        static void Disable();

        /**
         * @brief Pet (reset) the watchdog
         *
         * When in window mode, the petting must take place after the window open period, or the
         * device will be reset the same as if it never pet the watchdog in the first place.
         */
        static inline void Pet() {
            WDT->CLEAR.reg = WDT_CLEAR_CLEAR_KEY;
            __DSB();

            while(WDT->SYNCBUSY.bit.CLEAR) {}
        }

    private:
        static void Init();

    private:
        /// Task to notify for early warning interrupt
        static TaskHandle_t gEarlyWarningTask;
        /// Notification index to set for early warning interrupt
        static size_t gEarlyWarningNoteIndex;
        /// Bits to set for early warning interrupt
        static uintptr_t gEarlyWarningNoteBits;
};
}

#endif
