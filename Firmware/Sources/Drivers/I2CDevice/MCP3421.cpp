#include "MCP3421.h"
#include "Common.h"
#include "../I2CBus.h"

#include "Log/Logger.h"

#include <etl/array.h>

using namespace Drivers::I2CDevice;

/**
 * @brief Configure the ADC
 *
 * Set up the ADC in continuous conversion mode with the given configuration.
 */
MCP3421::MCP3421(Drivers::I2CBus *bus, const uint8_t address,
        const SampleDepth depth, const Gain gain) : bus(bus), deviceAddress(address),
    depth(depth), gain(gain) {
    int err = this->updateConfig();
    REQUIRE(!err, "%s: failed to write register %s (%d)", "MCP3421", "config", err);
}

/**
 * @brief Deinitialize the driver
 *
 * This puts the ADC into one shot mode, so it enders standby.
 */
MCP3421::~MCP3421() {
    this->isOneShot = true;

    int err = this->updateConfig();
    REQUIRE(!err, "%s: failed to write register %s (%d)", "MCP3421", "config", err);
}

/**
 * @brief Read the latest conversion
 *
 * Read the most recently converted code from the ADC. It's sign extended to fill out the entire
 * provided 32 bit variable.
 *
 * @param out Variable to receive the raw ADC code
 *
 * @return 0 on success, or a negative error code
 */
int MCP3421::read(int32_t &out) {
    int err;

    // read from device, based on the current sample depth
    etl::array<uint8_t, 3> buffer;
    const uint16_t bytesToRead = (this->depth == SampleDepth::Highest) ? 3 : 2;

    etl::array<I2CBus::Transaction, 1> txns{{
        {
            .address = this->deviceAddress,
            .read = 1,
            .length = bytesToRead,
            .data = buffer
        },
    }};

    err = this->bus->perform(txns);
    if(err) {
        return err;
    }

    // sign extend the result
    uint32_t temp;

    if(this->depth == SampleDepth::Highest) {
        temp = (static_cast<uint32_t>(buffer[0]) << 16) |
               (static_cast<uint32_t>(buffer[1]) << 8) |
               buffer[2];

        if(buffer[0] & 0b10) {
            temp |= 0xFF000000;
        }
    } else {
        temp = (static_cast<uint32_t>(buffer[0]) << 8) |
               buffer[1];

        if(buffer[0] & 0b10000000) {
            temp |= 0xFFFF0000;
        }
    }

    Logger::Trace("Read %02x %02x %02x", buffer[0], buffer[1], buffer[2]);

    out = static_cast<int32_t>(temp);
    return 0;
}

/**
 * @brief Write the converter configuration register
 *
 * Update the ADC's configuration register, based on the configured bit depth, gain, and one shot
 * configuration options.
 *
 * @return Status of underlying I²C transaction
 */
int MCP3421::updateConfig() {
    // build the config register
    uint8_t reg{0};

    if(!this->isOneShot) {
        reg |= (1 << 4);
    }

    reg |= (static_cast<uint8_t>(this->depth) & 0b11) << 2;
    reg |= (static_cast<uint8_t>(this->gain) & 0b11);

    // write it
    etl::array<I2CBus::Transaction, 1> txns{{
        {
            .address = this->deviceAddress,
            .read = 0,
            .length = 1,
            .data = {&reg, 1}
        },
    }};

    return this->bus->perform(txns);
}
