#ifndef APP_PINBALL_TASK_H
#define APP_PINBALL_TASK_H

#include <stddef.h>
#include <stdint.h>

#include "Rtos/Rtos.h"
#include "Util/Uuid.h"

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

    public:
        /**
         * @brief Task notification values
         */
        enum TaskNotifyBits: uint32_t {
            /**
             * @brief Front panel interrupt
             *
             * Indicates that the front panel interrupt line was asserted; we should poll its
             * hardware to figure out the reason.
             */
            FrontIrq                    = (1 << 0),

            /**
             * @brief Rear panel interrupt
             *
             * Indicates the rear panel's interrupt line was asserted.
             */
            RearIrq                     = (1 << 1),

            /**
             * @brief Power button pressed
             *
             * A falling edge (press) was detected on the power button.
             */
            PowerPressed                = (1 << 2),

            /**
             * @brief Encoder changed
             *
             * Either of the encoder inputs have changed; the encoder state machine should read
             * them out and update the UI state.
             */
            EncoderChanged              = (1 << 3),

            /**
             * @brief All valid notify bits
             *
             * Bitwise OR of all notification bits.
             */
            All                         = (FrontIrq | RearIrq | PowerPressed | EncoderChanged),
        };

    public:
        /**
         * @brief Notify task of interrupts
         *
         * Indicates to the task that interrupts occurred on either the front or rear IO busses.
         *
         * @param front Whether the front panel interrupt was asserted
         * @param rear Whether the rear IO interrupt was asserted
         *
         * @remark This method is not interrupt safe.
         */
        static void NotifyIrq(const bool front, const bool rear) {
            uint32_t bits{0};

            if(front) {
                bits |= TaskNotifyBits::FrontIrq;
            }
            if(rear) {
                bits |= TaskNotifyBits::RearIrq;
            }

            if(bits) {
                xTaskNotifyIndexed(gShared->task, kNotificationIndex, bits, eSetBits);
            }
        }

        /**
         * @brief Send a notification (from ISR)
         *
         * Notify the pinball task that some event happened, from within an ISR.
         *
         * @param bits Notification bits to set
         * @param woken Whether a higher priority task is woken
         */
        static void NotifyFromIsr(const TaskNotifyBits bits, BaseType_t *woken) {
            xTaskNotifyIndexedFromISR(gShared->task, kNotificationIndex,
                    static_cast<BaseType_t>(bits), eSetBits, woken);
        }

    private:
        Task();

        void main();
        void detectFrontPanel();

    private:
        /// Task handle
        TaskHandle_t task;

        /// Hardware revision of front panel board
        uint16_t frontRev{0};
        /// Front panel driver id
        Util::Uuid frontDriverId;

    private:
        /// Runtime priority level
        static const constexpr uint8_t kPriority{Rtos::TaskPriority::AppLow};

        /// Size of the task's stack, in words
        static const constexpr size_t kStackSize{420};
        /// Task name (for display purposes)
        static const constexpr etl::string_view kName{"Pinball"};
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
