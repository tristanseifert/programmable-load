#ifndef APP_MAIN_TASK_H
#define APP_MAIN_TASK_H

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

    public:
        /**
         * @brief Task notification bits
         *
         * Bit positions indicating a particular event took place on the main task. Each of these
         * will be set at the first app-specific index.
         *
         * Note that notifications are processed in the order they are specified in this enum.
         */
        enum TaskNotifyBits: uint32_t {
            /**
             * @brief IO bus interrupt
             *
             * The IO bus multiplexer indicates an interrupt has occurred on one of its secondary
             * busses, and needs to be serviced.
             */
            IoBusInterrupt              = (1 << 0),

            /**
             * @brief All valid notifications
             *
             * Bitwise OR of all notification values defined. These are in turn all bits that are
             * cleared after waiting for a notification.
             */
            All                         = (IoBusInterrupt),
        };

        /**
         * @brief Sends a notification to the main task.
         *
         * This indicates some sort of event took place, that the main task needs to act on. If the
         * event has additional data to it, it needs to be pushed on the appropriate message queue
         * before this call is made.
         *
         * @param bits Notification value to set; may be a bitwise OR of various notify bits.
         *
         * @remark This method should not be used from an interrupt handler; use NotifyTaskFromIsr
         *         instead.
         */
        inline static void NotifyTask(const TaskNotifyBits bits) {
            xTaskNotifyIndexed(gShared->task, kNotificationIndex, static_cast<uint32_t>(bits),
                    eSetBits);
        }

        /**
         * @brief Sends a notification to the main task, from an interrupt routine
         *
         * @param bits Notification value to set; may be a bitwise OR of various notify bits.
         * @param higherPriorityWoken Whether a higher priority task was unblocked
         */
        inline static void NotifyTaskFromIsr(const TaskNotifyBits bits, BaseType_t *higherPriorityWoken) {
            xTaskNotifyIndexedFromISR(gShared->task, kNotificationIndex,
                    static_cast<uint32_t>(bits), eSetBits, higherPriorityWoken);
        }

    private:
        Task();

        void main();

        void initHardware();
        void initNorFs();
        void discoverIoHardware();
        void discoverDriverHardware();
        void startApp();

    private:
        /**
         * @brief Main task handle
         *
         * This represents the app's main task.
         *
         * It is primarily used for sending direct-to-task notifications, which is a 32-bit word
         * that indicates various events have taken place.
         */
        TaskHandle_t task;

    private:
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

        /// Notification index
        static const constexpr size_t kNotificationIndex{Rtos::TaskNotifyIndex::TaskSpecific};

        /// Task information structure
        static StaticTask_t gTcb;
        /// Preallocated stack for the task
        static StackType_t gStack[kStackSize];
        /// Instance of the task class
        static Task *gShared;
};

void Start();
}

#endif
