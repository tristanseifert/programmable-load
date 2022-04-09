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
