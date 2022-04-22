#include "DAC60501.h"
#include "../I2CBus.h"

#include "Log/Logger.h"

#include <etl/array.h>

using namespace Drivers::I2CDevice;

/**
 * @brief Initialize DAC
 *
 * Reset the internal state of the DAC and configure the registers. The output code is not changed.
 */
DAC60501::DAC60501(Drivers::I2CBus *bus, const uint8_t address, const Gain gain) : bus(bus),
    deviceAddress(address) {
    int err;

    // reset device registers
    err = this->reset();
    REQUIRE(!err, "%s: %s (%d)", "DAC60501", "failed to reset device", err);

    // configure gain
    err = this->setGain(gain);
    REQUIRE(!err, "%s: %s (%d)", "DAC60501", "failed to set gain", err);
}

/**
 * @brief Update the DAC gain
 *
 * Half gain mode is realized by dividing the reference input; double gain by activating the output
 * buffer amplifier.
 */
int DAC60501::setGain(const Gain newGain) {
    uint16_t value;
    this->gain = newGain;

    switch(newGain) {
        case Gain::Half:
            // REF-DIV = 1, BUFF_GAIN = 0
            value = (1 << 8);
            break;
        case Gain::Unity:
            // REF-DIV = 1, BUFF_GAIN = 1
            value = (1 << 8) | (1 << 0);
            break;
        case Gain::Double:
            // REF-DIV = 0, BUFF_GAIN = 1
            value = (1 << 0);
            break;
    }

    return this->writeRegister(Reg::Gain, value);
}



/**
 * @brief Write DAC register
 *
 * Transfers 16 bits of data to a DAC register.
 */
int DAC60501::writeRegister(const Reg r, const uint16_t value) {
    // build request in buffer
    etl::array<uint8_t, 3> buffer{{
        static_cast<uint8_t>(r), static_cast<uint8_t>((value & 0xFF00) >> 8),
        static_cast<uint8_t>(value & 0x00FF)
    }};

    // write transaction
    etl::array<I2CBus::Transaction, 1> txns{{
        {
            .address = this->deviceAddress,
            .read = 0,
            .length = buffer.size(),
            .data = buffer
        },
    }};

    return this->bus->perform(txns);
}

/**
 * @brief Read DAC register
 *
 * Reads the DAC register specified.
 *
 * @param reg Register to read
 * @param outValue The 16-bit quantity read from the register
 */
int DAC60501::readRegister(const Reg r, uint16_t &outValue) {
    etl::array<uint8_t, 2> buffer;
    etl::array<uint8_t, 1> txBuffer{{
        static_cast<uint8_t>(r)
    }};

    // write transaction
    etl::array<I2CBus::Transaction, 2> txns{{
        {
            .address = this->deviceAddress,
            .read = 0,
            .length = txBuffer.size(),
            .data = txBuffer
        },
        {
            .address = this->deviceAddress,
            .read = 1,
            .continuation = 1,
            .length = buffer.size(),
            .data = buffer
        },
    }};

    const auto err = this->bus->perform(txns);
    if(err) {
        return err;
    }

    // extract value
    outValue = (buffer[0]) << 8 | (buffer[1]);

    return 0;
}

