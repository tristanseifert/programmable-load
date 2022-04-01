#include "PI4IOE5V9536.h"
#include "Common.h"

#include "Log/Logger.h"

using namespace Drivers::I2CDevice;

/**
 * @brief Initialize the IO expander
 *
 * All pins are configured as inputs (with optional inversion) or outputs.
 */
PI4IOE5V9536::PI4IOE5V9536(Drivers::I2CBus *bus, etl::span<const PinConfig, kIoLines> pins,
        const uint8_t address) : bus(bus), deviceAddress(address) {
    int err;
    uint8_t invert{0}, config{0};

    for(size_t i = 0; i < kIoLines; i++) {
        const auto &cfg = pins[i];
        const uint8_t bit = 1UL << static_cast<uint8_t>(i);

        // configure as input
        if(cfg.input) {
            config |= bit;

            if(cfg.invertInput) {
                invert |= bit;
            }
        }
        // configure as output
        else {
            config &= ~bit;

            if(cfg.initialOutput) {
                this->output |= bit;
            }
        }
    }

    err = this->writeRegister(Register::OutputPort, this->output);
    REQUIRE(!err, "%s: failed to write register %s (%d)", "PI4IOE5V9536", "OutputPort", err);
    err = this->writeRegister(Register::InputInvert, invert);
    REQUIRE(!err, "%s: failed to write register %s (%d)", "PI4IOE5V9536", "InputInvert", err);
    err = this->writeRegister(Register::PinConfig, config);
    REQUIRE(!err, "%s: failed to write register %s (%d)", "PI4IOE5V9536", "PinConfig", err);
}

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
int PI4IOE5V9536::setOutput(const uint8_t pin, const bool state) {
    if(pin >= kIoLines) {
        return Errors::InvalidPin;
    }

    // update shadow register
    const uint8_t bit = 1U << static_cast<uint8_t>(pin);
    if(state) {
        this->output |= bit;
    } else {
        this->output &= ~bit;
    }

    // write the register
    return this->writeRegister(Register::OutputPort, static_cast<uint8_t>(this->output & 0xFF));
}

