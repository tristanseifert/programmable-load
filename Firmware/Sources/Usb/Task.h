#ifndef USB_TASK_H
#define USB_TASK_H

#include <stddef.h>
#include <stdint.h>

#include "Rtos/Rtos.h"

#include <etl/string_view.h>

namespace UsbStack {
/**
 * @brief USB stack driver task
 *
 * All USB operations happen inside this driver task, including deferred interrupt handling. The
 * initialization of the stack also takes place here.
 */
class Task {
    public:
        static void Start();

    private:
        Task();

        void main();

    private:
        /// Task handle
        TaskHandle_t task;

        /// Priority level for the task
        static const constexpr uint8_t kPriority{Rtos::TaskPriority::Middleware};
        /// Size of the USB stack, in words
        static const constexpr size_t kStackSize{400};
        /// Task name (for display purposes)
        static const constexpr etl::string_view kName{"USBStack"};

        /// Task information structure
        static StaticTask_t gTcb;
        /// Preallocated stack for the task
        static StackType_t gStack[kStackSize];
};
}

#endif
