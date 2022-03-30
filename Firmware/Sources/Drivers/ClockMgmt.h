#ifndef DRIVERS_CLOCKMGMT_H
#define DRIVERS_CLOCKMGMT_H

#include <stdint.h>

namespace Drivers {
/**
 * @brief System clock management
 *
 * This is a combination driver for a few clocking-related peripherals, including GCLK.
 */
class ClockMgmt {
    public:
        /**
         * @brief Clock sources
         *
         * Each of the clock generators is set up early during chip initialization; each of them
         * then corresponds to a particular clock at a particular frequency. This enum defines the
         * names of each clock source (rather than their IDs) for peripherals.
         */
        enum class Clock: uint8_t {
            /**
             * @brief High speed clock (120 MHz) - GCLK 0
             *
             * Used for processor clock and high speed peripherals.
             *
             * Fed from DPLL0 (fed from XOSC1)
             */
            HighSpeed                   = 0,

            /**
             * @brief USB clock (48 MHz) - GCLK 1
             *
             * Fed from DFLL48M (fed from GCLK 5)
             */
            Usb                         = 1,

            /**
             * @brief Slow speed clock (32.768 kHz)
             *
             * Fed from OSCULP32K
             */
            LowSpeed                    = 3,

            /**
             * @brief External oscillator (12 Mhz)
             *
             * Fed from XOSC1, which is the external crystal.
             */
            ExternalClock               = 4,
        };

        /**
         * @brief Peripheral name
         *
         * Defines the peripheral (or groups of peripherals) that may be individually clocked. The
         * value of the enum is the associated PCHCTRL index.
         */
        enum class Peripheral: uint8_t {
            /**
             * @brief External interrupt controller
             */
            ExtIrq                      = 4,
        };

    public:
        static void EnableClock(const Peripheral periph, const Clock source);
        static void DisableClock(const Peripheral periph);
};
}

#endif
