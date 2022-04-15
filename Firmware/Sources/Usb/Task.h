#ifndef USB_TASK_H
#define USB_TASK_H

#include <stddef.h>
#include <stdint.h>

#include "Rtos/Rtos.h"

#include <etl/string_view.h>

namespace UsbStack {
namespace Vendor {
class InterfaceTask;
}

/**
 * @brief USB stack driver task
 *
 * All USB operations happen inside this driver task, including deferred interrupt handling. The
 * initialization of the stack also takes place here.
 *
 * Each of the USB interface/endpoints runs in their own task, which are launched after the core
 * USB stack has completed initialization; each of these tasks starts in a suspended mode, but will
 * "wake up" when the relevant interface is opened by a host.
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
        /// Vendor interface
        Vendor::InterfaceTask *vendorInterface{nullptr};

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
