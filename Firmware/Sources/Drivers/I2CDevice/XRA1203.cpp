#include "XRA1203.h"

#include "Drivers/I2CBus.h"
#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include <etl/array.h>

using namespace Drivers::I2CDevice;

/**
 * @brief Initialize the IO expander.
 *
 * All pins are configured according to the specified pin configuration map. These settings are
 * fixed, for the most part, until the part is reset/reinitialized.
 *
 * @param bus I²C bus to which the expander is connected
 * @param pins A list of 16 pin configuration entries, one for each IO pin
 * @param address I²C address of the device
 */
XRA1203::XRA1203(Drivers::I2CBus *bus, const uint8_t address,
        etl::span<const PinConfig, kIoLines> pins) : bus(bus), deviceAddress(address) {
    int err;
    uint16_t ocr{0}, pir{0}, gcr{0}, pur{0}, ier{0}, tscr{0}, reir{0}, feir{0}, ifr{0};

    for(size_t i = 0; i < kIoLines; i++) {
        const auto &cfg = pins[i];
        const uint16_t bit = 1U << static_cast<uint16_t>(i);

        // input specific
        if(cfg.input) {
            // enable as input
            gcr |= bit;

            // interrupts
            if(cfg.irq) {
                ier |= bit;
                if(cfg.irqRising) {
                    reir |= bit;
                }
                if(cfg.irqFalling) {
                    feir |= bit;
                }

                // irq filtering
                if(cfg.irqFilter) {
                    ifr |= bit;
                }
            }
            // pull ups
            if(cfg.pullUp) {
                pur |= bit;
            }
            // polarity inversion
            if(cfg.invertInput) {
                pir |= bit;
            }
        }
        // output specific
        else {
            // enable as output
            gcr &= ~bit;
            // initial output state
            if(cfg.initialOutput) {
                ocr |= bit;
            }
            // tristate
            if(cfg.tristated) {
                tscr |= bit;
            }
        }
    }

    // save shadow registers
    this->gpioConfig = gcr;
    this->output = ocr;
    this->tristate = tscr;

    // then write the actual registers
    err = this->writeRegister(Register::OCR1, ocr);
    REQUIRE(!err, "%s: failed to write register %s (%d)", "XRA1203", "OCR", err);
    err = this->writeRegister(Register::PIR1, pir);
    REQUIRE(!err, "%s: failed to write register %s (%d)", "XRA1203", "PIR", err);
    err = this->writeRegister(Register::PUR1, pur);
    REQUIRE(!err, "%s: failed to write register %s (%d)", "XRA1203", "PUR", err);
    err = this->writeRegister(Register::GCR1, gcr);
    REQUIRE(!err, "%s: failed to write register %s (%d)", "XRA1203", "GCR", err);
    err = this->writeRegister(Register::TSCR1, tscr);
    REQUIRE(!err, "%s: failed to write register %s (%d)", "XRA1203", "TSCR", err);
    err = this->writeRegister(Register::REIR1, reir);
    REQUIRE(!err, "%s: failed to write register %s (%d)", "XRA1203", "REIR", err);
    err = this->writeRegister(Register::FEIR1, feir);
    REQUIRE(!err, "%s: failed to write register %s (%d)", "XRA1203", "FEIR", err);
    err = this->writeRegister(Register::IFR1, ifr);
    REQUIRE(!err, "%s: failed to write register %s (%d)", "XRA1203", "IFR", err);
    err = this->writeRegister(Register::IER1, ier);
    REQUIRE(!err, "%s: failed to write register %s (%d)", "XRA1203", "IER", err);
}

/**
 * @brief Set the state of an output pin
 *
 * This internally updates the shadow register, then writes back the state for a bank of 8 pins at
 * a time to avoid the overhead of an extra read transaction to the device.
 *
 * @param pin Pin number to set ([0,15])
 * @param state New state of the pin; true = set
 *
 * @return 0 on success, or an error code
 */
int XRA1203::setOutput(const uint8_t pin, const bool state) {
    if(pin >= kIoLines) {
        return Errors::InvalidPin;
    }

    // update shadow register
    const uint16_t bit = 1U << static_cast<uint16_t>(pin);
    if(state) {
        this->output |= bit;
    } else {
        this->output &= ~bit;
    }

    // set the appropriate half of the register
    if(pin >= 8) {
        return this->writeRegister(Register::OCR1, static_cast<uint8_t>(this->output >> 8));
    } else {
        return this->writeRegister(Register::OCR2, static_cast<uint8_t>(this->output & 0xFF));
    }
}

/**
 * @brief Set whether a pin is tristated
 *
 * @param pin Pin number to set ([0,15])
 * @param isTristate Whether we tristate the pin
 *
 * @return 0 on success, or an error code
 */
int XRA1203::setOutputTristate(const uint8_t pin, const bool isTristate) {
    if(pin >= kIoLines) {
        return Errors::InvalidPin;
    }

    // update shadow register
    const uint16_t bit = 1U << static_cast<uint16_t>(pin);
    if(isTristate) {
        this->tristate |= bit;
    } else {
        this->tristate &= ~bit;
    }

    // set the appropriate half of the register
    if(pin >= 8) {
        return this->writeRegister(Register::TSCR1, static_cast<uint8_t>(this->tristate >> 8));
    } else {
        return this->writeRegister(Register::TSCR2, static_cast<uint8_t>(this->tristate & 0xFF));
    }
}



/**
 * @brief Write the upper and lower part of a register
 *
 * Writes the 16-bit value to two consecutive registers. All registers are laid out with the
 * register 1 having the high 8 bits, and register 2 the low 8 bits.
 *
 * @param reg Register to write
 * @param value 16-bit value to write to register
 *
 * @return 0 on success, or an error code
 */
int XRA1203::writeRegister(const Register reg, const uint16_t value) {
    etl::array<uint8_t, 3> request{
        static_cast<uint8_t>(reg), static_cast<uint8_t>((value & 0xFF00) >> 8),
        static_cast<uint8_t>(value & 0x00FF),
    };

    etl::array<I2CBus::Transaction, 1> txns{{
        {
            .address = this->deviceAddress,
            .read = 0,
            .length = 3,
            .data = request
        },
    }};

    return this->bus->perform(txns);
}

/**
 * @brief Read the upper and lower part of a register
 *
 * @param reg First (high) register to read
 * @param outValue Variable to receive the resulting value
 *
 * @return 0 on success, or an error code
 */
int XRA1203::readRegister(const Register reg, uint16_t &outValue) {
    int err;

    etl::array<uint8_t, 1> request{
        static_cast<uint8_t>(reg)
    };
    etl::array<uint8_t, 2> reply;

    etl::array<I2CBus::Transaction, 2> txns{{
        // write address
        {
            .address = this->deviceAddress,
            .read = 0,
            .length = request.size(),
            .data = request
        },
        // read two bytes of register data
        {
            .address = this->deviceAddress,
            .read = 1,
            .continuation = 1,
            .length = reply.size(),
            .data = reply
        },
    }};

    err = this->bus->perform(txns);

    if(!err) {
        outValue = (static_cast<uint16_t>(reply[0]) << 8) | reply[1];
    }

    return err;
}

