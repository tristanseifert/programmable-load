#ifndef APP_MAINTASK_H
#define APP_MAINTASK_H

#include <stddef.h>
#include <stdint.h>

#include "Rtos/Rtos.h"

#include <etl/string_view.h>

/// Main app task
namespace App::Main {
/**
 * @brief Main app task
 *
 * The main task is responsible for performing all hardware and app initialization. After the
 * initialization is complete, it receives messages from various other components (such as remote
 * controls and the local UI) to update the state of the system.
 */
class Task {
    friend void Start();

    private:
        Task();

        void main();

        void initHardware();
        void initOnboardPeripherals();
        void initNorFs();
        void discoverIoHardware();
        void discoverDriverHardware();
        void startApp();

        /// Task handle
        TaskHandle_t task;

        /**
         * @brief Initial priority level
         *
         * The task is created with this priority when initializing. Once initialization phase has
         * completed, we'll drop to kRunPriority.
         */
        static const constexpr uint8_t kPriority{Rtos::TaskPriority::Middleware};
        /// Runtime priority level
        static const constexpr uint8_t kRuntimePriority{Rtos::TaskPriority::AppLow};

        /// Size of the task stack, in words
        static const constexpr size_t kStackSize{420};
        /// Task name (for display purposes)
        static const constexpr etl::string_view kName{"AppMain"};

        /// Task information structure
        static StaticTask_t gTcb;
        /// Preallocated stack for the task
        static StackType_t gStack[kStackSize];
};

void Start();
}

#endif
