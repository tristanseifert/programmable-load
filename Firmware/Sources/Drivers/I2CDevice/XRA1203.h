#ifndef DRIVERS_I2CDEVICE_XRA1203_H
#define DRIVERS_I2CDEVICE_XRA1203_H

#include "Common.h"

#include <stdint.h>

#include <etl/span.h>

namespace Drivers {
class I2CBus;
}

namespace Drivers::I2CDevice {
/**
 * @brief XRA1203 – 16-bit IO expander with I²C interface and interrupts
 *
 * It supports 16 external IO lines, divided into two ports of 8 lines each. Each line can be
 * individually configured as an input (with optional, weak pull-up) or an output (which may be
 * tristated.)
 *
 * Input lines may each be configured to generate an interrupt on rising and/or falling edges of
 * the input signals; and additionally, each pin can individually have an input glitch filter
 * enabled or disabled.
 * */
class XRA1203 {
    public:
        /// Number of total IO lines
        constexpr static const size_t kIoLines{16};

        /**
         * @brief Error codes emitted by the device
         */
        enum Errors: int {
            /**
             * @brief Invalid pin number
             *
             * Specified pin number is invalid
             */
            InvalidPin                  = -5300,
        };


        /**
         * @brief Configuration for a single IO expander pin
         */
        struct PinConfig {
            /**
             * @brief Input mode
             *
             * Whether the pin is operating as an input (1) or output (0)
             */
            uint8_t input:1{1};
            /**
             * @brief Internal pullup resistor enable
             *
             * If the pin is configured as an input, the pin will have a weak (~100 kΩ) internal
             * resistor to VCC enabled.
             */
            uint8_t pullUp:1{0};
            /**
             * @brief Invert input polarity
             *
             * Invert the value read from the input state register for this pin.
             */
            uint8_t invertInput:1{0};
            /**
             * @brief Generate pin change interrupts
             *
             * Set to enable interrupts when the pin changes. It should be set in combination with
             * one of the rising or falling edge modes, or behavior is indeterminate and may
             * trigger on either edge.
             *
             * @remark Interrupts can only be generated for inputs.
             */
            uint8_t irq:1{0};
            /**
             * @brief Generate interrupt on rising edge
             *
             * Generate an interrupt when a rising edge is detected on the input.
             */
            uint8_t irqRising:1{0};
            /**
             * @brief Generate interrupt on falling edge
             *
             * Generate an interrupt when a falling edge is detected on the input.
             */
            uint8_t irqFalling:1{0};
            /**
             * @brief Interrupt filter enable
             *
             * When enabled, pulses on input lines must be greater than ~1µs to generate an
             * interrupt.
             */
            uint8_t irqFilter:1{1};
            /**
             * @brief Initial output state
             *
             * If the pin is configured as an output, the initial state of the pin.
             */
            uint8_t initialOutput:1{0};
            /**
             * @brief Tristate output pin
             *
             * If configured as an output, the pin can be tristated.
             */
            uint8_t tristated:1{0};
        };

        /**
         * @brief Pin configuration for an unused input
         *
         * Configures the pin as an input, with no interrupts or pull resistors.
         */
        constexpr static const PinConfig kPinConfigUnused{
            .input = true,
            .pullUp = false,
            .invertInput = false,
            .irq = false,
            .irqRising = false,
            .irqFalling = false,
            .irqFilter = false,
            .initialOutput = false,
            .tristated = true,
        };

    public:
        XRA1203(Drivers::I2CBus *bus, const uint8_t address,
               etl::span<const PinConfig, kIoLines> pins);

        int setOutput(const uint8_t pin, const bool state);
        int setOutputTristate(const uint8_t pin, const bool isTristate);

        /**
         * @brief Read the state of all pins
         *
         * Get the state of all pins, including the currently driven state of outputs, and that of
         * inputs (after applying the inversion, if enabled)
         *
         * @param outStates Variable to receive the pin state, where 1 = asserted
         *
         * @return 0 on success, or an error code
         */
        int readAllInputs(uint16_t &outStates) {
            return this->readRegister(Register::GSR1, outStates);
        }

    private:
        enum class Register: uint8_t {
            GSR1                        = 0x00,
            GSR2                        = 0x01,
            OCR1                        = 0x02,
            OCR2                        = 0x03,
            PIR1                        = 0x04,
            PIR2                        = 0x05,
            GCR1                        = 0x06,
            GCR2                        = 0x07,
            PUR1                        = 0x08,
            PUR2                        = 0x09,
            IER1                        = 0x0A,
            IER2                        = 0x0B,
            TSCR1                       = 0x0C,
            TSCR2                       = 0x0D,
            ISR1                        = 0x0E,
            ISR2                        = 0x0F,
            REIR1                       = 0x10,
            REIR2                       = 0x11,
            FEIR1                       = 0x12,
            FEIR2                       = 0x13,
            IFR1                        = 0x14,
            IFR2                        = 0x15,
        };

        /**
         * @brief Write a device register
         *
         * @param reg Device register to write
         * @param value Data to write into the register
         */
        int writeRegister(const Register reg, const uint8_t value) {
            return Common::WriteRegister(this->bus, this->deviceAddress, static_cast<uint8_t>(reg),
                    value);
        }
        /**
         * @brief Read a device register
         *
         * @param reg Device register to read
         * @param value Variable to receive register data
         */
        int readRegister(const Register reg, uint8_t &outValue) {
            return Common::ReadRegister(this->bus, this->deviceAddress, static_cast<uint8_t>(reg),
                    outValue);
        }

        int writeRegister(const Register reg, const uint16_t value);
        int readRegister(const Register reg, uint16_t &outValue);

    private:
        /// Parent bus
        Drivers::I2CBus *bus;
        /// Device address
        uint8_t deviceAddress;

        /**
         * @brief GPIO configuration
         *
         * Shadow of registers GCR1 and GCR2, for configuring the mode of an IO pin. A set bit
         * indicates the pin is configured as an input.
         */
        uint16_t gpioConfig{0xffff};
        /**
         * @brief Output value
         *
         * Shadow of registers OCR1 and OCR2, which hold the values written to output pins.
         */
        uint16_t output{0x0000};
        /**
         * @brief Output tristate
         *
         * Shadow of registers TSCR1 and TSCR2, which can set an output pin to be tristated if they
         * are set. They do not apply for inputs.
         */
        uint16_t tristate{0x0000};
};
}

#endif
