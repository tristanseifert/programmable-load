#ifndef APP_PINBALL_TASK_H
#define APP_PINBALL_TASK_H

#include <stddef.h>
#include <stdint.h>

#include "Rtos/Rtos.h"

#include <etl/string_view.h>

/// User interface task and related
namespace App::Pinball {
/**
 * @brief User interface task
 *
 * Handles dealing with user input (on the front panel) and updating the display and internal
 * state of the instrument. It's also responsible for updating the indicators on the front panel,
 * and handles the power button.
 */
class Task {
    friend void Start();

    private:
        Task();

        void main();

    private:
        /// Task handle
        TaskHandle_t task;

    private:
        /// Runtime priority level
        static const constexpr uint8_t kPriority{Rtos::TaskPriority::AppLow};

        /// Size of the task's stack, in words
        static const constexpr size_t kStackSize{420};
        /// Task name (for display purposes)
        static const constexpr etl::string_view kName{"Pinball"};

        /// Task information structure
        static StaticTask_t gTcb;
        /// Preallocated stack for the task
        static StackType_t gStack[kStackSize];
};

void Start();
}

#endif
