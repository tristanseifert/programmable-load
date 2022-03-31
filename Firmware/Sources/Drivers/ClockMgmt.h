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
            GCLK0                       = 0,
            GCLK1                       = 1,
            GCLK2                       = 2,
            GCLK3                       = 3,
            GCLK4                       = 4,
            GCLK5                       = 5,
            GCLK6                       = 6,
            GCLK7                       = 7,
            GCLK8                       = 8,
            GCLK9                       = 9,
            GCLK10                      = 10,
            GCLK11                      = 11,

            /**
             * @brief High speed clock (120 MHz) - GCLK 0
             *
             * Used for processor clock and high speed peripherals.
             *
             * Fed from DPLL0 (fed from XOSC1)
             */
            HighSpeed                   = GCLK0,

            /**
             * @brief USB clock (48 MHz) - GCLK 1
             *
             * Fed from DFLL48M (fed from GCLK 5)
             */
            Usb                         = GCLK1,

            /**
             * @brief Slow speed clock (32.768 kHz)
             *
             * Fed from OSCULP32K
             */
            LowSpeed                    = GCLK3,

            /**
             * @brief External oscillator (12 Mhz)
             *
             * Fed from XOSC1, which is the external crystal.
             */
            ExternalClock               = GCLK4,
        };

        /**
         * @brief Peripheral name
         *
         * Defines the peripheral (or groups of peripherals) that may be individually clocked. The
         * value of the enum is the associated PCHCTRL index.
         */
        enum class Peripheral: uint8_t {
            /**
             * @brief Shared slow clock
             *
             * A 32kHz clock shared between the FDPLL0/1 clock (for the internal lock timer), SDHC
             * slow, and SERCOM slow.
             */
            SharedSlow                  = 3,

            /**
             * @brief External interrupt controller
             */
            ExtIrq                      = 4,

            /// SERCOM0 primary core clock
            SERCOM0Core                 = 7,
            /// SERCOM1 primary core clock
            SERCOM1Core                 = 8,
            /// USB peripheral
            USBController               = 10,
            /// SERCOM2 primary core clock
            SERCOM2Core                 = 23,
            /// SERCOM3 primary core clock
            SERCOM3Core                 = 24,
            /// SERCOM4 primary core clock
            SERCOM4Core                 = 34,
            /// SERCOM5 primary core clock
            SERCOM5Core                 = 35,
        };

    public:
        static void EnableClock(const Peripheral periph, const Clock source);
        static void DisableClock(const Peripheral periph);
};
}

#endif
