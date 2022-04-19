#include "HmiDriver.h"
#include "../Task.h"

#include "Drivers/I2CBus.h"
#include "Drivers/I2CDevice/AT24CS32.h"
#include "Gui/InputManager.h"
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
    this->ioPollTimer = xTimerCreateStatic("HMI poller",
        // one-shot timer mode (we'll reload it as needed)
        pdMS_TO_TICKS(kIoPollTimerInterval), pdFALSE,
        this, [](auto timer) {
            Task::NotifyTask(Task::TaskNotifyBits::FrontIrq);
        }, &this->ioPollTimerStorage);
    REQUIRE(this->ioPollTimer, "HmiDriver: %s", "failed to allocate timer");
}

/**
 * @brief Tear down HMI driver resources
 *
 * Stops all of our background timers we created.
 */
HmiDriver::~HmiDriver() {
    xTimerDelete(this->ioPollTimer, 0);
}

/**
 * @brief Handles a front panel IRQ
 *
 * Read the IO expander to determine which buttons changed state.
 */
void HmiDriver::handleIrq() {
    int err;

    // reset the poll timer
    xTimerReset(this->ioPollTimer, 0);

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

    this->buttonState = static_cast<Button>(down);

    /*
     * Forward state changes in the mode and input enable buttons to our custom logic in the UI
     * task. Their behavior does not change depending on what's on screen.
     */
    if(newDown || newReleased) {
        Logger::Trace("down = %04x, up = %04x, state = %04x", newDown, newReleased, this->buttonState);
    }

    /*
     * Forward button events to the GUI layer. Note that we only do this for the select and menu
     * buttons: the other buttons are under the control of the UI task directly.
     */
    constexpr static const auto kGuiButtons{Button::MenuBtn | Button::Select};

    if((newDown & kGuiButtons) || (newReleased & kGuiButtons)) {
        Gui::InputKey guiDown{0}, guiUp{0};

        if(newDown & Button::MenuBtn) {
            guiDown |= Gui::InputKey::Menu;
        } if(newReleased & Button::MenuBtn) {
            guiUp |= Gui::InputKey::Menu;
        }

        if(newDown & Button::Select) {
            guiDown |= Gui::InputKey::Select;
        } if(newReleased & Button::Select) {
            guiUp |= Gui::InputKey::Select;
        }

        Gui::InputManager::KeyStateChanged(guiDown, guiUp);
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
int HmiDriver::setIndicatorState(const FrontIoIndicator state) {
    int err{0};

    // figure out what indicators changed
    const auto changed = static_cast<FrontIoIndicator>(state ^ this->indicatorState);

    // update indicators
    if(TestFlags(changed & FrontIoIndicator::Overheat)) {
        err = this->ledDriver.setBrightness(static_cast<uint8_t>(LedChannel::Overheat),
                TestFlags(state & FrontIoIndicator::Overheat) ? 1.f : 0.f);
        if(err) {
            goto beach;
        }
    }
    if(TestFlags(changed & FrontIoIndicator::Overcurrent)) {
        err = this->ledDriver.setBrightness(static_cast<uint8_t>(LedChannel::Overcurrent),
                TestFlags(state & FrontIoIndicator::Overcurrent) ? 1.f : 0.f);
        if(err) {
            goto beach;
        }
    }
    if(TestFlags(changed & FrontIoIndicator::GeneralError)) {
        err = this->ledDriver.setBrightness(static_cast<uint8_t>(LedChannel::Error),
                TestFlags(state & FrontIoIndicator::GeneralError) ? 1.f : 0.f);
        if(err) {
            goto beach;
        }
    }
    if(TestFlags(changed & FrontIoIndicator::LimitingOn)) {
        err = this->ledDriver.setBrightness(static_cast<uint8_t>(LedChannel::LimitingOn),
                TestFlags(state & FrontIoIndicator::LimitingOn) ? 1.f : 0.f);
        if(err) {
            goto beach;
        }
    }

    // mode selector
    if(TestFlags(changed & FrontIoIndicator::ModeCC)) {
        err = this->ledDriver.setBrightness(static_cast<uint8_t>(LedChannel::ModeCC),
                TestFlags(state & FrontIoIndicator::ModeCC) ? 1.f : 0.f);
        if(err) {
            goto beach;
        }
    }
    if(TestFlags(changed & FrontIoIndicator::ModeCV)) {
        err = this->ledDriver.setBrightness(static_cast<uint8_t>(LedChannel::ModeCV),
                TestFlags(state & FrontIoIndicator::ModeCV) ? 1.f : 0.f);
        if(err) {
            goto beach;
        }
    }
    if(TestFlags(changed & FrontIoIndicator::ModeCW)) {
        err = this->ledDriver.setBrightness(static_cast<uint8_t>(LedChannel::ModeCW),
                TestFlags(state & FrontIoIndicator::ModeCW) ? 1.f : 0.f);
        if(err) {
            goto beach;
        }
    }
    if(TestFlags(changed & FrontIoIndicator::ModeExt)) {
        err = this->ledDriver.setBrightness(static_cast<uint8_t>(LedChannel::ModeExt),
                TestFlags(state & FrontIoIndicator::ModeExt) ? 1.f : 0.f);
        if(err) {
            goto beach;
        }
    }

    // input enable button
    if(TestFlags(changed & FrontIoIndicator::InputEnabled)) {
        err = this->ledDriver.setBrightness(static_cast<uint8_t>(LedChannel::InputEnableG),
                TestFlags(state & FrontIoIndicator::InputEnabled) ? 1.f : 0.f);
        if(err) {
            goto beach;
        }

        err = this->ledDriver.setBrightness(static_cast<uint8_t>(LedChannel::InputEnableR),
                TestFlags(state & FrontIoIndicator::InputEnabled) ? 0.f : 1.f);
        if(err) {
            goto beach;
        }
    }

    // misc buttons
    if(TestFlags(changed & FrontIoIndicator::Menu)) {
        err = this->ledDriver.setBrightness(static_cast<uint8_t>(LedChannel::Menu),
                TestFlags(state & FrontIoIndicator::Menu) ? 1.f : 0.f);
        if(err) {
            goto beach;
        }
    }

    // if all settings succeeded, update our cached state
    this->indicatorState = state;

beach:;
    return err;
}

/**
 * @brief Set brightness of RGB status indicator
 */
int HmiDriver::setStatusColor(const uint32_t color) {
    int err;

    err = this->ledDriver.setBrightness(static_cast<uint8_t>(LedChannel::StatusR),
            static_cast<float>((color & 0xFF0000) >> 16) / 255.f);
    if(err) {
        goto beach;
    }

    err = this->ledDriver.setBrightness(static_cast<uint8_t>(LedChannel::StatusG),
            static_cast<float>((color & 0x00FF00) >> 8) / 255.f);
    if(err) {
        goto beach;
    }

    err = this->ledDriver.setBrightness(static_cast<uint8_t>(LedChannel::StatusB),
            static_cast<float>(color & 0x0000FF) / 255.f);
    if(err) {
        goto beach;
    }

beach:;
    return err;
}
