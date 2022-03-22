#include "EMC2101.h"

#include "Drivers/I2CBus.h"
#include "Log/Logger.h"

#include <etl/array.h>

using namespace Drivers::I2CDevice;

/**
 * @brief Initialize an EMC2101 fan controller
 *
 * The controller will be initialized with the fan off, and in manual control mode. Clients may
 * then choose to upload a fan control table to enter automatic mode. Note that it is possible to
 * exit automatic mode again later.
 *
 * @param bus I²C bus tow hich the controller is connected
 * @param conf Device configuration
 * @param address Optional slave address
 */
EMC2101::EMC2101(Drivers::I2CBus *bus, const Config &conf, const uint8_t address) :
    address(address), bus(bus) {
    // TODO: read product ID, manufacturer ID, revision

    // apply the configuration
    this->applyConfig(conf);
}

/**
 * @brief Initialize device registers
 *
 * Sets up the device registers with default values, based on the specified configuration.
 */
void EMC2101::applyConfig(const Config &conf) {
    int err;
    uint8_t temp;

    // copy some config values to our internal state
    this->invertPwm = conf.pwmPolarity;

    /*
     * Configuration register: Enable device, run fan in standby. Specify DAC or PWM mode based on
     * configuration.
     */
    temp = (conf.analogFan ? (1 << 4) : 0);

    err = this->writeRegister(Regs::Control, temp);
    REQUIRE(!err, "%s: failed to write register %s (%d)", "EMC2101", "Control", err);

    /*
     * Fan configuration: Fan lookup table is unused and unlocked for writing; 360kHz PWM clock
     * with system frequency divider is enabled. Use the specified PWM polarity.
     */
    temp = 0x03 | (1 << 2) | (1 << 5);
    temp |= (conf.pwmPolarity ? (1 << 4) : 0);

    err = this->writeRegister(Regs::FanConfig, temp);
    REQUIRE(!err, "%s: failed to write register %s (%d)", "EMC2101", "FanConfig", err);

    // fan lookup table hysteresis
    err = this->writeRegister(Regs::TableHysteresis, conf.autoHysteresis & 0x1f);
    REQUIRE(!err, "%s: failed to write register %s (%d)", "EMC2101", "TableHysteresis", err);

    /*
     * Set the conversion rate, fan spin-up behavior, PWM resolution, and digital filtering mode;
     * these are all fixed:
     *
     * - Register $04 = $08 (16 conversions/sec)
     * - Register $4B = $2D (fan spin up 50% for 800ms)
     * - Register $4D = $1F (PWM resolution)
     * - Register $4E = $01 (PWM frequency divider)
     * - Register $BF = $06 (Enable digital filtering at level 2 for external diode)
     *
     */
    err = this->writeRegister(Regs::ConversionRate, 0x08);
    REQUIRE(!err, "%s: failed to write register %s (%d)", "EMC2101", "ConversionRate", err);

    err = this->writeRegister(Regs::FanSpinup, 0x2D);
    REQUIRE(!err, "%s: failed to write register %s (%d)", "EMC2101", "FanSpinup", err);

    err = this->writeRegister(Regs::PwmFrequency, 0x1F);
    REQUIRE(!err, "%s: failed to write register %s (%d)", "EMC2101", "PwmFrequency", err);
    err = this->writeRegister(Regs::PwmFreqDivide, 0x01);
    REQUIRE(!err, "%s: failed to write register %s (%d)", "EMC2101", "PwmFreqDivide", err);

    err = this->writeRegister(Regs::AvgFilter, 0x06);
    REQUIRE(!err, "%s: failed to write register %s (%d)", "EMC2101", "Averaging", err);

    /*
     * Calculate the TACH limit for the fan's minimum RPM, if specified.
     */
    if(conf.minRpm) {
        const uint16_t tachMin = (5'400'000UL / static_cast<uint32_t>(conf.minRpm));

        err = this->writeRegister(Regs::TachLimitLow, (tachMin & 0xFF));
        REQUIRE(!err, "%s: failed to write register %s (%d)", "EMC2101", "TachLimitLow", err);

        err = this->writeRegister(Regs::TachLimitHigh, ((tachMin >> 8) & 0xFF));
        REQUIRE(!err, "%s: failed to write register %s (%d)", "EMC2101", "TachLimitHigh", err);
    }
}

/**
 * @brief Write a register in the device.
 *
 * This performs the "write byte" protocol. This consists of sending the register address and
 * register data in one write transaction.
 *
 * @param reg Register number to write
 * @param value Data to write to the register
 *
 * @return 0 on success, or underlying bus error code
 */
int EMC2101::writeRegister(const Regs reg, const uint8_t value) {
    // build up the request
    etl::array<uint8_t, 2> data{{
        static_cast<uint8_t>(reg), value
    }};

    // send it
    return this->bus->perform({{
        {
            .address = this->address,
            .read = 0,
            .length = 2,
            .data = data
        }
    }});
}

/**
 * @brief Read a register in the device.
 *
 * This performs the "read byte" protocol. This consists of sending the register address,
 * performing a restart, then reading one byte.
 *
 * @param reg Register number to read
 * @param outValue Variable to receive the register data
 *
 * @return 0 on success, or underlying bus error code
 */
int EMC2101::readRegister(const Regs reg, uint8_t &outValue) {
    int err;

    // build up the request
    etl::array<uint8_t, 1> request{{
        static_cast<uint8_t>(reg)
    }}, response;

    // send it
    err = this->bus->perform({{
        {
            .address = this->address,
            .read = 0,
            .length = 1,
            .data = request
        },
        {
            .address = this->address,
            .read = 1,
            .continuation = 1,
            .length = 1,
            .data = response
        }
    }});

    if(!err) {
        outValue = response[0];
    }

    return err;
}



/**
 * @brief Read the internal temperature sensor
 *
 * @param outExternalTemp Variable to receive the internal temperature
 *
 * @return 0 on success, error code otherwise
 */
int EMC2101::getInternalTemp(float &outInternalTemp) {
    int err;
    uint8_t high;

    // read register
    err = this->readRegister(Regs::InternalTemp, high);
    if(err) {
        return err;
    }

    // convert
    outInternalTemp = static_cast<int8_t>(high);
    return 0;
}

/**
 * @brief Read the external temperature sensor
 *
 * The external temperature sensor has an up to 0.125°C resolution.
 *
 * @param outExternalTemp Variable to receive the external temperature
 *
 * @return 0 on success, error code otherwise
 */
int EMC2101::getExternalTemp(float &outExternalTemp) {
    int err;
    uint8_t low, high;

    // read register (both halves)
    err = this->readRegister(Regs::ExternalTempHigh, high);
    if(err) {
        return err;
    }

    err = this->readRegister(Regs::ExternalTempLow, low);
    if(err) {
        return err;
    }

    // piece the value together
    float temp = static_cast<int8_t>(high);

    temp += (0.125f * (low >> 5));

    outExternalTemp = temp;
    return 0;
}

/**
 * @brief Read the current fan speed.
 *
 * This reads the tachometer count register, then converts the reading to RPM.
 *
 * @param outRpm Variable to receive the fan speed, or -1 if the tachometer reading is invalid
 *
 * @return 0 on success, error code otherwise
 */
int EMC2101::getFanSpeed(unsigned int &outRpm) {
    int err;
    uint8_t low, high;

    // read register (both halves)
    err = this->readRegister(Regs::TachCountLow, low);
    if(err) {
        return err;
    }

    err = this->readRegister(Regs::TachCountHigh, high);
    if(err) {
        return err;
    }

    // do the calculation
    const uint32_t val{(static_cast<uint32_t>(high) << 8) | static_cast<uint32_t>(low)};

    if(val == 0xFFFF) {
        outRpm = -1;
    } else {
        outRpm = 5'400'000U / val;
    }

    return 0;
}

/**
 * @brief Write the autonomous fan control map
 *
 * Transfer to the fan controller the specified fan map, which is a list of temperature/fan speed
 * pairs, ordered in increasing temperature. A hysterisis (configured during initialization) is
 * applied to all measurements.
 *
 * @param map The fan map to upload to the device. A maximum of 8 entries can be specified; unused
 *        entries should be at the end of the with a temperature value of 0x7f and a fan speed of
 *        0x3f.
 *
 * @return 0 on success, or an error code.
 *
 * @remark Entries in the map should be specified in increasing order of temperature.
 */
int EMC2101::setFanMap(etl::span<const FanMapEntry> map) {
    int err;

    // ensure we're in manual control mode
    if(this->useFanTable) {
        return Errors::InvalidMode;
    }

    // validate entries
    if(map.size() < 2) {
        return Errors::InvalidMap;
    }

    for(size_t i = 1; i < map.size(); i++) {
        const auto &prev = map[i - 1], &current = map[i];

        if(prev.temp > current.temp) {
            return Errors::InvalidMap;
        }
    }

    // write each entry
    for(size_t i = 0; i < map.size(); i++) {
        const auto &entry = map[i];
        const auto regOff{i * 2};

        // write temperature
        err = this->writeRegister(static_cast<Regs>(static_cast<uint8_t>(Regs::TableTemp1)+regOff),
                entry.temp & 0x7f);
        if(err) {
            return err;
        }

        // write speed
        err = this->writeRegister(static_cast<Regs>(static_cast<uint8_t>(Regs::TableSpeed1)+regOff),
                entry.speed & 0x3f);
        if(err) {
            return err;
        }
    }

    // if we get here, the table was written successfully
    return 0;
}

/**
 * @brief Set the fan mode
 *
 * @param automatic Whether the fan controller uses the programmed fan control curve to drive the
 *        fan, or the manually specified fan value.
 *
 * @return 0 on success
 *
 * @remark When disabling the automatic fan control mode, the controller will continue to drive the
 *         fan at the most recent speed, until manually changed.
 */
int EMC2101::setFanMode(const bool automatic) {
    int err;
    uint8_t temp;

    // fixed settings and polarity (see comments in applyConfig)
    temp = 0x03 | (1 << 2);
    temp |= (this->invertPwm ? (1 << 4) : 0);

    // fan control mode
    if(!automatic) {
        temp |= (1 << 5);
    }

    // write it
    err = this->writeRegister(Regs::FanConfig, temp);
    if(!err) {
        this->useFanTable = automatic;
    }
    return err;
}

/**
 * @brief Set the current fan speed
 *
 * @param speed Fan speed, where 0 is off, and 0xFF is maximum speed.
 *
 * @return 0 on success
 *
 * @remark This only works when the fan controller is _not_ using the automatic fan lookup table
 *         control mode.
 */
int EMC2101::setFanSpeed(const uint8_t speed) {
    // ensure we're in manual mode
    if(this->useFanTable) {
        return Errors::InvalidMode;
    }

    // write register
    return this->writeRegister(Regs::FanSetting, speed >> 2);
}

