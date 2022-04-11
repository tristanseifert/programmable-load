#include "InputManager.h"
#include "ScreenManager.h"
#include "WorkQueue.h"

#include "Log/Logger.h"
#include "Rtos/Rtos.h"

using namespace Gui;

InputManager *InputManager::gShared{nullptr};

/**
 * @brief Initialize shared input manager
 */
void InputManager::Init() {
    static uint8_t gBuf[sizeof(InputManager)] __attribute__((aligned(alignof(InputManager))));
    auto ptr = reinterpret_cast<InputManager *>(gBuf);

    gShared = new (ptr) InputManager();
}

/**
 * @brief Initialize the input manager
 *
 * This creates any timers needed to detect long button presses.
 */
InputManager::InputManager() {
    this->menuLongPressTimer = xTimerCreateStatic("GUI menu btn timer",
        // one-shot timer mode (we'll reload it as needed)
        pdMS_TO_TICKS(kMenuHoldPeriod), pdFALSE,
        this, [](auto timer) {
            reinterpret_cast<InputManager *>(pvTimerGetTimerID(timer))->handleMenuLongPress();
        }, &this->menuLongPressTimerBuf);
    REQUIRE(this->menuLongPressTimer, "gui: %s", "failed to allocate timer");
}

/**
 * @brief Stop all timers and release resources
 */
InputManager::~InputManager() {
    xTimerDelete(this->menuLongPressTimer, 0);
}



/**
 * @brief Handle a state change on the button inputs.
 *
 * When keys are pushed down at first, we'll start any long press timers. When the key is released,
 * we invoke the appropriate action (and cancel the timer.) If the button is held long enough for
 * the timer to fire, that sets a flag and when it is eventually released, we just do nothing.
 *
 * Most UI actions are triggered when a button is released.
 *
 * @remark This will be called in the context of the GUI task.
 */
void InputManager::updateKeys(const InputKey pressed, const InputKey released) {
    BaseType_t ok;

    // handle pressed keys
    if(TestFlags(pressed & InputKey::Menu)) {
        ok = xTimerReset(this->menuLongPressTimer, 0);
        REQUIRE(ok == pdPASS, "gui: %s", "failed to re-arm timer");
    }

    // handle released keys
    if(TestFlags(released & InputKey::Menu)) {
        xTimerStop(this->menuLongPressTimer, 0);

        if(!TestFlags(this->longPressFired & InputKey::Menu)) {
            // TODO: invoke menu action in screen manager
            Logger::Notice("gui: %s", "Regular press on menu!");
        }
        this->longPressFired &= ~InputKey::Menu;
    }
    if(TestFlags(released & InputKey::Select)) {

    }
}

/**
 * @brief Handle a long press on the menu button
 *
 * This brings up the nav menu.
 *
 * @remark This is called in the context of the OS timer task, so we need to send a notification
 *         on the GUI work queue here.
 */
void InputManager::handleMenuLongPress() {
    this->longPressFired |= InputKey::Menu;

    Logger::Notice("gui: %s", "Long press on menu!");

    WorkQueue::Submit([](auto ctx) {
        ScreenManager::OpenNavStackMenu();
    }, this);
}



/**
 * @brief Forward encoder events
 *
 * Indicates the rotary encoder has changed by the given number of "clicks" where negative values
 * indicate counterclockwise rotation.
 *
 * @param delta Relative encoder change since last invocation
 */
void InputManager::updateEncoder(const int delta) {
    // TODO: implement
    Logger::Notice("gui: encoder delta = %d", delta);
}
