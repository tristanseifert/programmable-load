#ifndef DRIVERS_SERCOMBASE_H
#define DRIVERS_SERCOMBASE_H

#include <stddef.h>
#include <stdint.h>

#include <vendor/sam.h>

namespace Drivers {
/**
 * @brief SERCOM utilities
 *
 * Provides some utilities common to all SERCOM-based serial drivers, as well as interrupt
 * dispatching.
 */
class SercomBase {
    friend class I2C;
    friend class Spi;

    friend void ::SERCOM0_0_Handler();
    friend void ::SERCOM0_1_Handler();
    friend void ::SERCOM0_2_Handler();
    friend void ::SERCOM0_3_Handler();

    friend void ::SERCOM1_0_Handler();
    friend void ::SERCOM1_1_Handler();
    friend void ::SERCOM1_2_Handler();
    friend void ::SERCOM1_3_Handler();

    friend void ::SERCOM2_0_Handler();
    friend void ::SERCOM2_1_Handler();
    friend void ::SERCOM2_2_Handler();
    friend void ::SERCOM2_3_Handler();

    friend void ::SERCOM3_0_Handler();
    friend void ::SERCOM3_1_Handler();
    friend void ::SERCOM3_2_Handler();
    friend void ::SERCOM3_3_Handler();

    friend void ::SERCOM4_0_Handler();
    friend void ::SERCOM4_1_Handler();
    friend void ::SERCOM4_2_Handler();
    friend void ::SERCOM4_3_Handler();

    friend void ::SERCOM5_0_Handler();
    friend void ::SERCOM5_1_Handler();
    friend void ::SERCOM5_2_Handler();
    friend void ::SERCOM5_3_Handler();

    public:
        /**
         * @brief Identifies a SERCOM instance
         *
         * The chip has multiple instances of SERCOM, each of which are identical, except for the
         * IO pins that they can drive, as well as possible clocking restrictions.
         */
        enum class Unit: uint8_t {
            Unit0                       = 0,
            Unit1                       = 1,
            Unit2                       = 2,
            Unit3                       = 3,
            Unit4                       = 4,
            Unit5                       = 5,
        };

        /**
         * @brief SERCOM CTRLA Mode values
         *
         * Defines the values for the SERCOM peripheral mode for a particular interface type.
         */
        enum class Mode: uint8_t {
            UsartExternalClk            = 0x0,
            UsartInternalClk            = 0x1,
            SpiSlave                    = 0x2,
            SpiMaster                   = 0x3,
            I2CSlave                    = 0x4,
            I2CMaster                   = 0x5,
        };

        SercomBase() = delete;

    private:
        static void RegisterHandler(const Unit unit, const uint8_t irq,
                void (*fn)(void *ctx), void *ctx = nullptr);

        static void MarkAsUsed(const Unit unit);
        static void MarkAsAvailable(const Unit unit);

        static void SetApbClock(const Unit unit, const bool state);

        /**
         * @brief Get offset into IRQ handler array
         *
         * Calculates the offset of a particular interrupt in the irq handler table.
         *
         * @param unit SERCOM unit ([0, 5])
         * @param irq Interrupt number for this SERCOM ([0, 3])
         */
        constexpr static inline size_t HandlerOffset(const uint8_t unit, const uint8_t irq) {
            return (unit * 0x4) + (irq & 0x3);
        }

        /**
         * @brief Get the IRQ number for the given unit's interrupt line.
         *
         * @param unit SERCOM unit ([0, 5])
         * @param irq Interrupt number for this SERCOM ([0, 3])
         */
        constexpr static inline auto GetIrqVector(const Unit unit, const uint8_t irq) {
            return kHandlerIrqn[HandlerOffset(static_cast<uint8_t>(unit), irq)];
        }

        /**
         * @brief Get the DMA trigger for the unit's receive event
         *
         * @param unit SERCOM unit ([0, 5])
         * @return Trigger value (for DMA CHCTRLA.TRIGSRC) for SERCOM RX trigger
         */
        constexpr static inline uint8_t GetDmaRxTrigger(const Unit unit) {
            return kDmaRxTriggers[static_cast<size_t>(unit)];
        }

        /**
         * @brief Get the DMA trigger for the unit's transmit event
         *
         * @param unit SERCOM unit ([0, 5])
         * @return Trigger value (for DMA CHCTRLA.TRIGSRC) for SERCOM TX trigger
         */
        constexpr static inline uint8_t GetDmaTxTrigger(const Unit unit) {
            return kDmaTxTriggers[static_cast<size_t>(unit)];
        }

        /**
         * @brief Get register base for unit
         *
         * @param unit SERCOM unit ([0, 5])
         *
         * @return Peripheral register base address
         */
        constexpr static inline ::Sercom *MmioFor(const Unit unit) {
            switch(unit) {
                case Unit::Unit0:
                    return SERCOM0;
                case Unit::Unit1:
                    return SERCOM1;
                case Unit::Unit2:
                    return SERCOM2;
                case Unit::Unit3:
                    return SERCOM3;
                case Unit::Unit4:
                    return SERCOM4;
                case Unit::Unit5:
                    return SERCOM5;
            }
        }

        /**
         * @brief Get core clock for unit
         *
         * @param unit SERCOM unit ([0, 5])
         *
         * @return SERCOM core clock frequency, in Hz, or 0 if unknown
         */
        constexpr static inline uint32_t CoreClockFor(const Unit unit) {
            return kFastClocks[static_cast<size_t>(unit)];
        }

        /**
         * @brief Get slow clock for unit
         *
         * @param unit SERCOM unit ([0, 5])
         *
         * @return SERCOM slow clock frequency, in Hz, or 0 if unknown
         */
        constexpr static inline uint32_t SlowClockFor(const Unit unit) {
            return kSlowClocks[static_cast<size_t>(unit)];
        }

    private:
        /**
         * @brief SERCOM interrupt handler
         */
        struct Handler {
            /// Function to invoke for this interrupt
            void (*fn)(void *ctx);
            /// Argument to pass to the function
            void *ctx{nullptr};

            /**
             * @brief Check whether the handler has been installed.
             *
             * An installed handler has a non-NULL function pointer.
             */
            operator bool() const {
                return !!this->fn;
            }

            /**
             * Clear the handler.
             */
            void reset() {
                this->fn = nullptr;
                this->ctx = nullptr;
            }
        };

        /**
         * @brief Number of SERCOM units in the device
         */
        constexpr static const size_t kNumUnits{6};

        /**
         * @brief Number of interrupt handlers to reserve space for
         *
         * Each SERCOM has 4 interrupt vectors. Currently, we only have 6 SERCOM instances.
         */
        constexpr static const size_t kNumHandlers{4*kNumUnits};

        /**
         * @brief SERCOM fast clocks
         *
         * Contains the fast clocks for each SERCOM unit, or 0 if unknown.
         */
        static const uint32_t kFastClocks[kNumUnits];

        /**
         * @brief SERCOM slow clocks
         *
         * Contains the slow clocks for each SERCOM unit, or 0 if unknown. Some SERCOM drivers do
         * not need to use the slow clock.
         */
        static const uint32_t kSlowClocks[kNumUnits];

        /**
         * @brief Interrupt vectors
         *
         * Maps an interrupt handler to the corresponding hardware vector, such as is required to
         * configure the NVIC. Use the same HandlerOffset here.
         */
        constexpr static const IRQn_Type kHandlerIrqn[kNumHandlers]{
            SERCOM0_0_IRQn, SERCOM0_1_IRQn, SERCOM0_2_IRQn, SERCOM0_3_IRQn,
            SERCOM1_0_IRQn, SERCOM1_1_IRQn, SERCOM1_2_IRQn, SERCOM1_3_IRQn,
            SERCOM2_0_IRQn, SERCOM2_1_IRQn, SERCOM2_2_IRQn, SERCOM2_3_IRQn,
            SERCOM3_0_IRQn, SERCOM3_1_IRQn, SERCOM3_2_IRQn, SERCOM3_3_IRQn,
            SERCOM4_0_IRQn, SERCOM4_1_IRQn, SERCOM4_2_IRQn, SERCOM4_3_IRQn,
            SERCOM5_0_IRQn, SERCOM5_1_IRQn, SERCOM5_2_IRQn, SERCOM5_3_IRQn,
        };

        /**
         * @brief DMA RX triggers
         */
        constexpr static const uint8_t kDmaRxTriggers[kNumUnits] {
            0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e//, 0x10, 0x12
        };

        /**
         * @brief DMA TX triggers
         */
        constexpr static const uint8_t kDmaTxTriggers[kNumUnits] {
            0x05, 0x07, 0x09, 0x0b, 0x0d, 0x0f//, 0x11, 0x13
        };

        /**
         * @brief Interrupt handler routines
         *
         * Since the SERCOM interupts are general to the instance, rather than the type of
         * protocol (SPI, I2C, UART) running on it, we need to do a layer of indirect dispatching
         * with these handler functions.
         */
        static Handler gHandlers[kNumHandlers];

        /**
         * @brief Bitfield indicating which SERCOM units are used
         */
        static uint32_t gUsed;

        /**
         * @brief Whether the slow clock for the SERCOM block has been enabled
         */
        static bool gSlowClockEnabled;
};
}

#endif
