#include "HmiDriver.h"

#include "Drivers/I2CBus.h"
#include "Drivers/I2CDevice/AT24CS32.h"
#include "Log/Logger.h"
#include "Util/InventoryRom.h"
#include "Rtos/Rtos.h"

using namespace App::Pinball;

const Util::Uuid HmiDriver::kDriverId(kUuidBytes);

/**
 * @brief Initialize the HMI
 *
 * This sets up the IO expander and LED driver at their default addresses.
 */
HmiDriver::HmiDriver(Drivers::I2CBus *bus, Drivers::I2CDevice::AT24CS32 &idprom) : 
    FrontIoDriver(bus, idprom), 
    ledDriver(Drivers::I2CDevice::PCA9955B(bus, kLedDriverAddress, kLedDriverRefCurrent,
                kLedConfig)),
    ioExpander(Drivers::I2CDevice::XRA1203(bus, kExpanderAddress, kPinConfigs)) {
}

/**
 * @brief Tear down HMI driver resources
 *
 * Stops all of our background timers we created.
 */
HmiDriver::~HmiDriver() {

}

/**
 * @brief Handles a front panel IRQ
 *
 * Read the IO expander to determine which buttons changed state.
 */
void HmiDriver::handleIrq() {
    int err;

    // read the raw IO state
    uint16_t inputs;

    err = this->ioExpander.readAllInputs(inputs);
    REQUIRE(!err, "HmiDriver: %s (%d)", "failed to read expander state");

    inputs &= kIoButtonMask;

    // figure out which buttons are currently down
    uintptr_t down{0};

    if(inputs & kIoButtonMenu) {
        down |= Button::MenuBtn;
    }
    if(inputs & kIoButtonSelect) {
        down |= Button::Select;
    }
    if(inputs & kIoButtonInputEnable) {
        down |= Button::InputBtn;
    }
    if(inputs & kIoButtonModeCC) {
        down |= Button::ModeSelectCC;
    }
    if(inputs & kIoButtonModeCV) {
        down |= Button::ModeSelectCV;
    }
    if(inputs & kIoButtonModeCW) {
        down |= Button::ModeSelectCW;
    }
    if(inputs & kIoButtonModeExt) {
        down |= Button::ModeSelectExt;
    }

    // figure out which buttons were pressed and released
    const uintptr_t newDown = (down & (~this->buttonState));
    const uintptr_t newReleased = (down ^ this->buttonState) & ~down;

    // invoke callbacks
    this->buttonState = static_cast<Button>(down);

    if(newDown || newReleased) {
        // TODO: call into GUI code
        Logger::Trace("down = %04x, up = %04x, state = %04x", newDown, newReleased, this->buttonState);
    }
}

/**
 * @brief Update indicator state
 *
 * Updates the state of which LEDs should be illuminated on the front panel, then writes the
 * appropriate registers in the LED driver.
 *
 * TODO: handle gradation/blinking as needed
 */
int HmiDriver::setIndicatorState(const Indicator state) {
    int err{0};

    // figure out what indicators changed
    const uintptr_t changed = (state ^ this->indicatorState);

    // update indicators
    if(changed & Indicator::Overheat) {
        err = this->ledDriver.setBrightness(static_cast<uint8_t>(LedChannel::Overheat),
                (state & Indicator::Overheat) ? 1.f : 0.f);
        if(err) {
            goto beach;
        }
    }
    if(changed & Indicator::Overcurrent) {
        err = this->ledDriver.setBrightness(static_cast<uint8_t>(LedChannel::Overcurrent),
                (state & Indicator::Overcurrent) ? 1.f : 0.f);
        if(err) {
            goto beach;
        }
    }
    if(changed & Indicator::GeneralError) {
        err = this->ledDriver.setBrightness(static_cast<uint8_t>(LedChannel::Error),
                (state & Indicator::GeneralError) ? 1.f : 0.f);
        if(err) {
            goto beach;
        }
    }
    if(changed & Indicator::LimitingOn) {
        err = this->ledDriver.setBrightness(static_cast<uint8_t>(LedChannel::LimitingOn),
                (state & Indicator::LimitingOn) ? 1.f : 0.f);
        if(err) {
            goto beach;
        }
    }

    // mode selector
    if(changed & Indicator::ModeCC) {
        err = this->ledDriver.setBrightness(static_cast<uint8_t>(LedChannel::ModeCC),
                (state & Indicator::ModeCC) ? 1.f : 0.f);
        if(err) {
            goto beach;
        }
    }
    if(changed & Indicator::ModeCV) {
        err = this->ledDriver.setBrightness(static_cast<uint8_t>(LedChannel::ModeCV),
                (state & Indicator::ModeCV) ? 1.f : 0.f);
        if(err) {
            goto beach;
        }
    }
    if(changed & Indicator::ModeCW) {
        err = this->ledDriver.setBrightness(static_cast<uint8_t>(LedChannel::ModeCW),
                (state & Indicator::ModeCW) ? 1.f : 0.f);
        if(err) {
            goto beach;
        }
    }
    if(changed & Indicator::ModeExt) {
        err = this->ledDriver.setBrightness(static_cast<uint8_t>(LedChannel::ModeExt),
                (state & Indicator::ModeExt) ? 1.f : 0.f);
        if(err) {
            goto beach;
        }
    }

    // input enable button
    if(changed & Indicator::InputEnabled) {
        err = this->ledDriver.setBrightness(static_cast<uint8_t>(LedChannel::InputEnableG),
                (state & Indicator::InputEnabled) ? 1.f : 0.f);
        if(err) {
            goto beach;
        }

        err = this->ledDriver.setBrightness(static_cast<uint8_t>(LedChannel::InputEnableR),
                (state & Indicator::InputEnabled) ? 0.f : 1.f);
        if(err) {
            goto beach;
        }
    }

    // misc buttons
    if(changed & Indicator::Menu) {
        err = this->ledDriver.setBrightness(static_cast<uint8_t>(LedChannel::Menu),
                (state & Indicator::Menu) ? 1.f : 0.f);
        if(err) {
            goto beach;
        }
    }

    // if all settings succeeded, update our cached state
    this->indicatorState = state;

beach:;
    return err;
}

