#ifndef DRIVERS_EXTERNALIRQ_H
#define DRIVERS_EXTERNALIRQ_H

#include <stddef.h>
#include <stdint.h>

#include <vendor/sam.h>

namespace Drivers {
/**
 * @brief EIC: External Interrupt Controller
 *
 * Generate events/interrupts from external IO lines. Most all IO pads can be routed to one of the
 * 16 EXTINT inputs into the controller, where programmable filtering and debouncing can be
 * applied.
 *
 * @remark The EIC is configured to always use the ultra-low power 32kHz clock for filters, rather
 *         than any high speed clocks. This ensures that we can always generate interrupts for the
 *         IO pins, even if the system is in sleep mode and clocks are stopped.
 */
class ExternalIrq {
    public:
        /**
         * @brief Interrupt sense mode
         *
         * Defines the events on the signal that will trigger the interrupt.
         */
        enum class SenseMode: uint8_t {
            /// No events are triggered
            None                        = 0,
            /// Edge triggered on rising edge
            EdgeRising                  = 1,
            /// Edge triggered on falling edge
            EdgeFalling                 = 2,
            /// Edge triggered, both edges
            EdgeBoth                    = 3,
            /// Level triggered, high
            LevelHigh                   = 4,
            /// Level triggered, low
            LevelLow                    = 5,
        };

        /**
         * @brief Configuration for an external interrupt line
         *
         * This wraps up all of the configurable options for an external interrupt line.
         */
        struct Config {
            /**
             * @brief Enable interrupt
             *
             * Whether this external interrupt line is capable of generating interrupts.
             *
             * @remark You still have to enable and configure the IRQn in the NVIC to actually
             *         receive the interrupt.
             */
            uint8_t irq:1{1};
            /**
             * @brief Enable event
             *
             * Whether this external interrupt line triggers an event
             */
            uint8_t event:1{0};

            /**
             * @brief Whether the filter is enabled
             *
             * When set, the input filter (clocked by the EIC clock) is applied to this external
             * interrupt line.
             */
            uint8_t filter:1{0};
            /**
             * @brief Debounce input
             *
             * Controls whether the input is debounced or not. All pins with debouncing use the
             * standard debounce filter configuration applied during initialization.
             */
            uint8_t debounce:1{0};

            /**
             * @brief Sense mode
             *
             * The mode used by the hardware to trigger on the input signal; it defines whether the
             * interrupt is edge or level triggered.
             */
            SenseMode mode{SenseMode::None};
        };

    public:
        ExternalIrq() = delete;

        static void Init();

        static void ConfigureLine(const uint8_t line, const Config &conf);

        /**
         * @brief Irq handler helper
         *
         * Invoke this to check the appropriate interrupt flag bit.
         *
         * @return Whether the interrupt actually fired or if it was a spurious interrupt.
         */
        static inline bool HandleIrq(const uint8_t line) {
            if(EIC->INTFLAG.reg & (1U << static_cast<uint32_t>(line))) {
                EIC->INTFLAG.reg = (1U << static_cast<uint32_t>(line));
                return true;
            }

            return false;
        }

    private:
        static void Reset();
        static void Enable();
        static void Disable();

    private:
        /**
         * @brief Enable timeout
         *
         * Number of cycles to wait for the enable bit to synchronize. If this timeout expires, we
         * assume something is wrong with the peripheral.
         */
        constexpr static const size_t kEnableSyncTimeout{1000};

        /**
         * @brief Is the controller enabled?
         *
         * Most all configuration registers may only be altered when the controller is disabled, so
         * keep track of this here. This avoids having to query (and potentially synchronize with)
         * the hardware registers.
         */
        static bool gEnabled;

        /**
         * @brief Bitmask indicating which inputs are enabled
         *
         * For each of the 16 EXTINT input lines, a corresponding bit in this field will be set if
         * the port is enabled.
         */
        static uint16_t gLinesEnabled;
};
}

#endif
