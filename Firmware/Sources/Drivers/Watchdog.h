#ifndef DRIVER_WATCHDOG_H
#define DRIVER_WATCHDOG_H

#include <stddef.h>
#include <stdint.h>

#include "Rtos/Rtos.h"

extern "C" {
void WWDG1_IRQHandler();
}

namespace Drivers {
/**
 * @brief Watchdog driver
 *
 * Provides support for the system watchdog, in both regular and windowed mode. The early warning
 * interrupt may be enabled, which can be used to notify a chosen task.
 */
class Watchdog {
    friend void ::WWDG1_IRQHandler();

    public:
        /**
         * @brief Clock divider
         *
         * Settings possible for the watchdog's internal clock dividers. The clock is divided off
         * a 1.024kHz reference, which is derived from the internal ultra low power 32kHz
         * oscillator. This means it's not totally accurate.
         */
        enum class ClockDivider: uint8_t {
            Div1                        = 0x0,
            Div2                        = 0x1,
            Div4                        = 0x2,
            Div8                        = 0x3,
            Div16                       = 0x4,
            Div32                       = 0x5,
            Div64                       = 0x6,
            Div128                      = 0x7,
        };

        /// Watchdog configuration
        struct Config {
            /**
             * @brief Watchdog clock divider
             *
             * Indirectly defines the period of the watchdog count; this is an additional
             * division on top of the existing /4096 from the APB1 input clock. This divided clock
             * then drives the watchdog counter.
             */
            ClockDivider divider;

            /**
             * @brief Watchdog timeout
             *
             * A 7-bit value that defines the watchdog period. It's in units of the watchdog clock
             * count frequency; this counter is decremented by one every tick of that clock, and
             * when it reaches 0x3f, a reset is generated.
             */
            uint8_t counter{0x7f};

            /**
             * @brief Window value
             *
             * This is the upper bound above which the watchdog will generate a reset when it is
             * petted.(The lower bound is fixed at 0x3f.)
             */
            uint8_t windowValue{0x7f};

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

        /**
         * @brief Pet (reset) the watchdog
         *
         * When in window mode, the petting must take place after the window open period, or the
         * device will be reset the same as if it never pet the watchdog in the first place.
         */
        static inline void Pet() {
            WWDG1->CR = WWDG_CR_WDGA | (static_cast<uint32_t>(gCounterValue) << WWDG_CR_T_Pos);
            __DSB();
        }

    private:
        /// Task to notify for early warning interrupt
        static TaskHandle_t gEarlyWarningTask;
        /// Notification index to set for early warning interrupt
        static size_t gEarlyWarningNoteIndex;
        /// Bits to set for early warning interrupt
        static uintptr_t gEarlyWarningNoteBits;

        /// Down counter value
        static uint8_t gCounterValue;
        /// Initial counter value
        static uint8_t gWindowValue;
};
}

#endif
