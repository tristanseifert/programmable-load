#ifndef SUPERVISOR_TASK_H
#define SUPERVISOR_TASK_H

#include <stddef.h>
#include <etl/string_view.h>

#include "Rtos/Rtos.h"

namespace Supervisor {
/**
 * @brief Runtime supervisor task
 *
 * This encapsulates the supervisor's runtime state as well.
 */
class Task {
    friend void Init();

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
             * @brief Watchdog early warning interrupt
             *
             * Indicates the watchdog's early warning interrupt has fired. We'll see if all
             * required tasks have checked in, and if so, pet the watchdog to avoid a system
             * reset.
             */
            WatchdogWarning             = (1 << 1),

            /**
             * @brief All valid notifications
             *
             * Bitwise OR of all notification values defined. These are in turn all bits that are
             * cleared after waiting for a notification.
             */
            All                         = (WatchdogWarning),
        };

        Task();
        ~Task();

    private:
        void main();

        void wdgEarlyWarning();

    private:
        /// priority of the supervisor task
        static const constexpr uint8_t kPriority{Rtos::TaskPriority::Middleware};
        /// Size of the task stack, in words
        static const constexpr size_t kStackSize{300};
        /// Task name (for display purposes)
        static const constexpr etl::string_view kName{"Supervisor"};
        /// Notification index
        static const constexpr size_t kNotificationIndex{Rtos::TaskNotifyIndex::TaskSpecific};

        /// Whether the manual checkin timer is enabled
        static const constexpr bool kUseTimer{false};
        /// Checkin interval, in milliseconds
        static const constexpr size_t kCheckinInterval{200};

        /// Supervisor task
        TaskHandle_t handle;
        /// Periodic checkin timer
        TimerHandle_t checkinTimer;

        /// number of successful checkins
        size_t numSuccessfulCheckins{0};
};
}

#endif
