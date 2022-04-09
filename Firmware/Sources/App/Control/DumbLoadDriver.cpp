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
    voltageAdc(bus, kVSenseAdcAddress, kVSenseAdcBits) {
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
int DumbLoadDriver::setEnabled(const bool isEnabled) {
    // TODO: implement
    return -1;
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
    int err, scaledVoltage;

    // read ADC as voltage
    err = this->voltageAdc.readVoltage(scaledVoltage);
    if(err) {
        return err;
    }

    // scale based on frontend amp (it has a 1:50 gain)
    outVoltage = scaledVoltage;

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

beach:;
    // return status code
    return err;
}

