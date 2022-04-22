#include "DumbLoadDriver.h"
#include "Task.h"

#include "Drivers/I2CBus.h"
#include "Drivers/I2CDevice/AT24CS32.h"
#include "Drivers/I2CDevice/MCP3421.h"
#include "Drivers/I2CDevice/PI4IOE5V9536.h"
#include "Log/Logger.h"
#include "Util/InventoryRom.h"
#include "Rtos/Rtos.h"

using namespace App::Control;

const Util::Uuid DumbLoadDriver::kDriverId(kUuidBytes);

/**
 * @brief Initialize the dumb load pcb
 *
 * Configures all peripherals on the board.
 */
DumbLoadDriver::DumbLoadDriver(Drivers::I2CBus *bus, Drivers::I2CDevice::AT24CS32 &idprom) : LoadDriver(bus, idprom),
    ioExpander(bus, kExpanderPinConfig, kExpanderAddress),
    voltageAdc(bus, kVSenseAdcAddress, kVSenseAdcBits),
    currentAdc1(bus, kCurrentAdc1Address, kCurrentAdcBits), currentDac1(bus, kCurrentDac1Address) {
    int err;

    /*
     * Read some more data out of the IDPROM. This consists of the following:
     *
     * - Atom 0x40: Maximum allowed input voltage and current
     *
     * TODO: read calibration data
     */
    err = Util::InventoryRom::GetAtoms(
            // TODO: take length into account
            [](auto addr, auto len, auto buf, auto ctx) -> int {
        return reinterpret_cast<Drivers::I2CDevice::AT24CS32 *>(ctx)->readData(addr, buf);
    }, &idprom,
    [](auto header, auto ctx, auto readBuf) -> bool {
        static etl::array<uint8_t, 8> gLimitsBuf;

        switch(header.type) {
            case Util::InventoryRom::AtomType::DriverRating:
                readBuf = gLimitsBuf;
                break;

            default:
                break;
        }
        return true;
    }, this,
    [](auto header, auto buffer, auto ctx) {
        auto inst = reinterpret_cast<DumbLoadDriver *>(ctx);

        switch(header.type) {
            /*
             * The maximum voltage/current supported by the driver is encoded in this atom, as two
             * consecutive 32-bit big endian integers.
             */
            case Util::InventoryRom::AtomType::DriverRating:
                uint32_t temp;

                memcpy(&temp, buffer.data(), 4);
                inst->maxVoltage = __builtin_bswap32(temp);
                memcpy(&temp, buffer.template subspan<4>().data(), 4);
                inst->maxCurrent = __builtin_bswap32(temp);
                break;

            default:
                break;
        }
    }, this);

    Logger::Notice("DumbLoadDriver: Vmax = %u mV, Imax = %u mA", this->maxVoltage,
            this->maxCurrent);
    REQUIRE(this->maxVoltage && this->maxCurrent, "DumbLoadDriver: %s", "invalid maximum ratings");

    // set up timer used to deenergize relay coil
    static StaticTimer_t gRelayTimer;
    this->relayTimer = xTimerCreateStatic("Load Relay Timer",
            // one-shot timer mode
            pdMS_TO_TICKS(kRelayPulseWidth), pdFALSE,
            // timer ID is this object
            this, [](auto timer) {
                auto driver = reinterpret_cast<DumbLoadDriver *>(pvTimerGetTimerID(timer));

                driver->deenergizeRelays = true;
                Task::NotifyTask(Task::TaskNotifyBits::IrqAsserted);
            }, &gRelayTimer);
    REQUIRE(this->relayTimer, "DumbLoadDriver: %s", "failed to allocate load relay timer");

    // initialization complete, blink the indicator
    err = this->setIndicatorState(true);
    if(err) {
        Logger::Error("DumbLoadDriver: %s (%d)", "failed to set indicator", err);
    }

    vTaskDelay(pdMS_TO_TICKS(200));

    err = this->setIndicatorState(false);
    if(err) {
        Logger::Error("DumbLoadDriver: %s (%d)", "failed to set indicator", err);
    }

    // use internal sense
    this->setExternalVSense(false);
}

/**
 * @brief Deinitialize the driver
 *
 * Disable the load current and place converters in standby mode.
 */
DumbLoadDriver::~DumbLoadDriver() {
    int err;

    // disable the indicator
    err = this->setIndicatorState(false);
    if(err) {
        Logger::Error("DumbLoadDriver: %s (%d)", "failed to set indicator", err);
    }
}



/**
 * @brief Interrupt handler
 *
 * We don't actually have any hardware interrupts; this is used for software interrupts, currently
 * only triggered from timers to de-energize the relay coils of the VSense relay.
 */
void DumbLoadDriver::handleIrq() {
    int err;

    // handle the latching relays
    if(this->deenergizeRelays) {
        err = this->ioExpander.clearOutputs((1 << kRelaySetPin) | (1 << kRelayResetPin));
        REQUIRE(!err, "DumbLoadDriver: %s (%d)", "failed to reset relays", err);

        this->deenergizeRelays = false;
    }
}



/**
 * @brief Enable or disable the load
 *
 * This works by gating the current set DACs to output an all 0's code when the driver is to be
 * disabled.
 *
 * @return 0 on success, error code otherwise
 */
int DumbLoadDriver::setEnabled(const bool enable) {
    // bail if no state change
    if(enable == this->isEnabled) {
        return 0;
    }

    // set current to zero to disable load
    if(!enable) {
        this->isEnabled = false;
        return this->setOutputCurrent(0, true);
    }
    // when re-enabling, restore the old current value
    else {
        this->isEnabled = true;
        return this->setOutputCurrent(this->currentSetpoint, true);
    }
}

/**
 * @brief Sample current ADCs
 *
 * Reads the current values from the two current ADCs, the nconverts these values to current
 * values. If they are out of range, we'll adjust the ADC's PGA accordingly.
 */
int DumbLoadDriver::readInputCurrent(uint32_t &outCurrent) {
    int err;
    uint32_t current{0}, tempCurrent;

    // read ADC 1
    err = this->readCurrentAdc(this->currentAdc1, tempCurrent);
    if(err) {
        return err;
    }

    current += tempCurrent;

    // read all ADCs
    outCurrent = current;
    return 0;
}

/**
 * @brief Read the ADC value and convert to current
 *
 * This may change the gain setting of the converter, if needed. The gain is increased if the
 * read value is below a certain threshold, and decreased if it's above a certain threshold.
 *
 * @param adc Converter to read from
 * @param outCurrent Converted current value, in µA
 *
 * @return 0 on succes or a negative error code
 *
 * @TODO This should be smarter (to handle noise, for example) about gain changes
 */
int DumbLoadDriver::readCurrentAdc(Drivers::I2CDevice::MCP3421 &adc, uint32_t &outCurrent) {
    int err, voltage;
    uint16_t sample;

//sample:;
    // read the raw value (µV)
    err = adc.readVoltage(voltage, sample);
    if(err) {
        return err;
    }

    /*
     * Adjust the gain if needed.
     *
     * Note that we ignore values of 0 for gain changes, since that probably means that the load
     * is disabled.
     *
     * @TODO: Make the thresholds based on the gain setting
     */
    constexpr static const uint16_t kLowerThreshold{0x100};
    constexpr static const uint16_t kUpperThreshold{0xf00};

    if(sample) {
        const auto oldGain = adc.getGain();
        auto gain = oldGain;

        if(sample >= kUpperThreshold) {
            // reduce the gain
            gain = Drivers::I2CDevice::MCP3421::LowerGain(gain);
        } else if(sample <= kLowerThreshold) {
            // increase the gain
            gain = Drivers::I2CDevice::MCP3421::HigherGain(gain);
        }

        // update gain
        if(oldGain != gain) {
            Logger::Notice("Change gain: %u -> %u", oldGain, gain);

            err = adc.setGain(gain);
            if(err) {
                return err;
            }
        }
    }

    /*
     * Convert to current
     *
     * TODO: apply calibration/compensation, and handle different sense resistor values
     */
    outCurrent = static_cast<float>(voltage) / kSenseResistance;

    return 0;
}

/**
 * @brief Set the output current
 *
 * Update the current drive settings.
 *
 * @param newCurrent Output current (in µA)
 * @param isInternal Whether the caller is internal; in this case we don't check that we're enabled
 *        and also don't update the current setting value.
 */
int DumbLoadDriver::setOutputCurrent(const uint32_t current, const bool isInternal) {
    int err;

    // ensure we're enabled
    if(!isInternal) {
        // if not, just record the current setting (to apply it when re-enabling)
        if(!this->isEnabled) {
            this->currentSetpoint = current;
            return 0;
        }
    }

    /*
     * Calculate the desired current setting value, in µV.
     *
     * The hardware works by matching the MOSFET drive strength so the current sense resistor
     * voltage drop is equal to the control voltage. So we just need to calculate what the actual
     * voltage across the resistor would be for the desired current.
     */
    const auto microvolts = static_cast<float>(current) * kSenseResistance;

    /*
     * Update the drive DACs.
     *
     * For voltage settings above 1V, use 1x gain. Otherwise, use 2x gain to get some additional
     * resolution for the drive voltage.
     */
    Drivers::I2CDevice::DAC60501::Gain newGain;
    float percent{0.f};

    newGain = Drivers::I2CDevice::DAC60501::Gain::Unity;
    percent = microvolts / kDacReference;

    // set the gains and DAC voltages
    if(this->currentDac1.getGain() != newGain) {
        err = this->currentDac1.setGain(newGain);
        if(err) {
            return err;
        }
    }

    err = this->currentDac1(percent);
    if(err) {
        return err;
    }

    // assume success
    if(!isInternal) {
        this->currentSetpoint = current;
    }
    return 0;
}




/**
 * @brief Read the current input voltage
 *
 * Read the most recent conversion from the voltage sense ADC, then convert it to a voltage.
 *
 * @param outVoltage Input voltage, in mV
 *
 * @return 0 on success, error code otherwise
 */
int DumbLoadDriver::readInputVoltage(uint32_t &outVoltage) {
    int err, senseVoltage;

    // read ADC as voltage
    err = this->voltageAdc.readVoltage(senseVoltage);
    if(err) {
        return err;
    }

    // scale based on frontend amp (it has a 1:50 gain)
    outVoltage = (static_cast<float>(senseVoltage) * kVSenseGain) / 1000.f;

    // TODO: update PGA/scale/gain as needed
    return 0;
}


/**
 * @brief Set the VSense input source
 *
 * Drive the latching relay to reset (sense from input) or set (external input) and then arms the
 * timer to deenergize the relay.
 *
 * @param isExternal Use external input
 *
 * @return 0 on success or an error code
 */
int DumbLoadDriver::setExternalVSense(const bool isExternal) {
    int err;

    // first, kill the relays if any of them are being driven
    err = this->ioExpander.clearOutputs((1 << kRelaySetPin) | (1 << kRelayResetPin));
    if(err) {
        goto beach;
    }

    // set the appropriate relay coil and start the timer
    err = this->ioExpander.setOutput(isExternal ? kRelaySetPin : kRelayResetPin, true);
    if(err) {
        goto beach;
    }

    err = xTimerReset(this->relayTimer, 0);
    if(err == pdFAIL) {
        // ensure the relays are disabled again
        err = this->ioExpander.clearOutputs((1 << kRelaySetPin) | (1 << kRelayResetPin));
        REQUIRE(!err, "DumbLoadDriver: %s (%d)", "failed to reset relays", err);

        err = -1;
        goto beach;
    }
    err = 0;

beach:;
    // return status code
    return err;
}

