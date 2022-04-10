#ifndef APP_CONTROL_TASK_H
#define APP_CONTROL_TASK_H

#include <stddef.h>
#include <stdint.h>

#include "Rtos/Rtos.h"
#include "Util/Uuid.h"

#include <etl/string_view.h>

namespace App::Control {
class LoadDriver;

class Task {
    friend void Start();

    public:
        /**
         * @brief Task notification values
         */
        enum TaskNotifyBits: uint32_t {
            /**
             * @brief External trigger event
             *
             * An edge was detected on the external trigger input.
             */
            ExternalTrigger             = (1 << 0),
            /**
             * @brief Driver interrupt
             *
             * The driver board asserted its interrupt line
             */
            IrqAsserted                 = (1 << 1),

            /**
             * @brief All valid notify bits
             *
             * Bitwise OR of all notification bits.
             */
            All                         = (ExternalTrigger | IrqAsserted),
        };

    public:
        Task();

        /**
         * @brief Send a notification (from ISR)
         *
         * Notify the task that some event happened, from within an ISR.
         *
         * @param bits Notification bits to set
         * @param woken Whether a higher priority task is woken
         */
        inline static void NotifyFromIsr(const TaskNotifyBits bits, BaseType_t *woken) {
            xTaskNotifyIndexedFromISR(gShared->task, kNotificationIndex,
                    static_cast<BaseType_t>(bits), eSetBits, woken);
        }

        /**
         * @brief Send a notification
         *
         * Notify the control loop task some event happened.
         *
         * @param bits Notification bits to set
         */
        inline static void NotifyTask(const TaskNotifyBits bits) {
            xTaskNotifyIndexed(gShared->task, kNotificationIndex, static_cast<BaseType_t>(bits),
                    eSetBits);
        }


    private:
        void main();

        void identifyDriver();

    private:
        /// Task handle
        TaskHandle_t task;

        /// Driver handling the load
        LoadDriver *driver{nullptr};
        /// Driver identifier
        Util::Uuid driverId;
        /// Hardware revision of driver
        uint16_t pcbRev{0};

    private:
        /// Runtime priority level
        static const constexpr uint8_t kPriority{Rtos::TaskPriority::AppHigh};

        /// Size of the task's stack, in words
        static const constexpr size_t kStackSize{420};
        /// Task name (for display purposes)
        static const constexpr etl::string_view kName{"Control"};
        /// Notification index
        static const constexpr size_t kNotificationIndex{Rtos::TaskNotifyIndex::TaskSpecific};

        /// Task information structure
        static StaticTask_t gTcb;
        /// Preallocated stack for the task
        static StackType_t gStack[kStackSize];

        /// Shared task instance
        static Task *gShared;
};

void Start();
}

#endif
