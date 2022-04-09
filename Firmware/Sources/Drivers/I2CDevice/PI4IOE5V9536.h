#ifndef DRIVERS_I2CDEVICE_PI4IOE5V9536_H
#define DRIVERS_I2CDEVICE_PI4IOE5V9536_H

#include "Common.h"

#include <stdint.h>

#include <etl/span.h>

namespace Drivers {
class I2CBus;
}

namespace Drivers::I2CDevice {
/**
 * @brief PI4IOE5V9536 – 4-bit IO expander with I²C interface
 *
 * Provides four IO lines, which may be configured as either an input or an output line. There is
 * no support for interrupts, input filtering, nor tristated outputs.
 */
class PI4IOE5V9536 {
    public:
        /// Number of total IO lines
        constexpr static const size_t kIoLines{4};

        /**
         * @brief Error codes emitted by the device
         */
        enum Errors: int {
            /**
             * @brief Invalid pin number
             *
             * Specified pin number is invalid. Valid pin numbers are [0, 3].
             */
            InvalidPin                  = -5310,
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
             * @brief Invert input polarity
             *
             * Invert the value read from the input state register for this pin.
             */
            uint8_t invertInput:1{0};
            /**
             * @brief Initial output state
             *
             * If the pin is configured as an output, the initial state of the pin.
             */
            uint8_t initialOutput:1{0};
        };

        /**
         * @brief Pin configuration for an unused input
         *
         * Configures the pin as an input
         */
        constexpr static const PinConfig kPinConfigUnused{
            .input = true,
            .invertInput = false,
            .initialOutput = false,
        };

    public:
        PI4IOE5V9536(Drivers::I2CBus *bus, etl::span<const PinConfig, kIoLines> pins,
                const uint8_t address = 0b100'0001);

        /**
         * @brief Set the state of an output pin
         *
         * This internally updates the shadow register, then writes back the state of all outputs.
         *
         * @param pin Pin number to set ([0,3])
         * @param state New state of the pin; true = set
         *
         * @return 0 on success, or an error code
         */
        inline int setOutput(const uint8_t pin, const bool state) {
            if(state) {
                return this->setOutputs((1 << pin));
            } else {
                return this->clearOutputs((1 << pin));
            }
        }

        /**
         * @brief Set all output bits specified
         *
         * This internally updates the shadow register, then writes back the state of all outputs.
         *
         * @param bits Bitmask of pins to set
         *
         * @return 0 on success, or an error code
         */
        int setOutputs(const uint8_t bits) {
            if(bits >= (1 << kIoLines)) {
                return Errors::InvalidPin;
            }

            this->output |= bits;
            return this->writeRegister(Register::OutputPort, static_cast<uint8_t>(this->output & 0xFF));
        }

        /**
         * @brief Clear all output bits specified
         *
         * This internally updates the shadow register, then writes back the state of all outputs.
         *
         * @param bits Bitmask of pins to clear
         *
         * @return 0 on success, or an error code
         */
        int clearOutputs(const uint8_t bits) {
            if(bits >= (1 << kIoLines)) {
                return Errors::InvalidPin;
            }

            this->output &= ~bits;
            return this->writeRegister(Register::OutputPort, static_cast<uint8_t>(this->output & 0xFF));
        }

        /**
         * @brief Read the state of all pins
         *
         * Get the state of all pins, including the currently driven state of outputs, and that of
         * inputs (after applying the inversion, if enabled)
         *
         * @param outStates Variable to receive the pin state, where 1 = asserted. Only the low 4
         *        bits are valid.
         *
         * @return 0 on success, or an error code
         */
        int readAllInputs(uint8_t &outStates) {
            return this->readRegister(Register::InputPort, outStates);
        }

    private:
        enum class Register: uint8_t {
            InputPort                   = 0x00,
            OutputPort                  = 0x01,
            InputInvert                 = 0x02,
            PinConfig                   = 0x03,
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

    private:
        /// Parent bus
        Drivers::I2CBus *bus;
        /// Device address
        uint8_t deviceAddress;

        /**
         * @brief Output value
         *
         * Shadow of the output port value, to allow setting the state of an individual output
         * pin without needing to do a readback from the device.
         */
        uint8_t output{0};
};
};

#endif
