#ifndef GUI_INPUTMANAGER_H
#define GUI_INPUTMANAGER_H

#include <bitflags.h>
#include <stddef.h>
#include <stdint.h>

#include "Rtos/Rtos.h"

namespace Gui {
/**
 * @brief Physical GUI keys
 *
 * An enum of keys, which may be bitwise-ORed together, that the GUI layer is concerned
 * with.
 */
enum class InputKey: uintptr_t {
    Menu                        = (1 << 0),
    Select                      = (1 << 1),
};
ENUM_FLAGS_EX(InputKey, uintptr_t);


/**
 * @brief Handles user input directed at the GUI layer
 *
 * This collects input from keys (menu, select) and the rotary encoder (for scrolling) and then
 * distributes the events throughout the GUI layer.
 */
class InputManager {
    public:
        static void Init();

        /// Inform the input manager that a key was pressed or released
        static void KeyStateChanged(const InputKey pressed, const InputKey released) {
            gShared->updateKeys(pressed, released);
        }
        /// Inform the input manager that encoder state changed
        static void EncoderChanged(const int delta) {
            gShared->updateEncoder(delta);
        }

    private:
        InputManager();
        ~InputManager();

        void updateKeys(const InputKey pressed, const InputKey released);
        void handleMenuLongPress();

        void updateEncoder(const int delta);

    private:
        /// How long menu button should be held to trigger a long press, in msec
        constexpr static const size_t kMenuHoldPeriod{1250};
        /**
         * @brief Menu long press timer
         *
         * This timer is used to detect a long press (hold) on the menu button, which will always
         * trigger opening the nav stack menu.
         */
        TimerHandle_t menuLongPressTimer;
        StaticTimer_t menuLongPressTimerBuf;

        /// Buttons for which a long press timer fired
        InputKey longPressFired{0};

    private:
        static InputManager *gShared;
};
}

#endif
