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
             * @brief Update control data
             *
             * Updates the locally read data from the analog driver board (that is, its input
             * voltage, current, and any error state).
             *
             * This would be fired by a background timer.
             */
            SampleData                  = (1 << 2),

            /// Update the external sense input relay
            UpdateSenseRelay            = (1 << 3),

            /**
             * @brief All valid notify bits
             *
             * Bitwise OR of all notification bits.
             */
            All                         = (ExternalTrigger | IrqAsserted | SampleData |
                    UpdateSenseRelay),
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

        /**
         * @brief Get the current input voltage
         *
         * @return Voltage at input terminals, in millivolts
         */
        inline static auto GetInputVoltage() {
            return gShared->inputVoltage;
        }

        /**
         * @brief Get input current
         *
         * @return Current through the load, in microamps
         */
        inline static auto GetInputCurrent() {
            return gShared->inputCurrent;
        }

        /**
         * @brief Determine whether the external or integrated voltage sense is used
         *
         * @return If external voltage sense is active
         */
        inline static auto GetIsExternalSenseActive() {
            return gShared->isUsingExternalSense;
        }

        /**
         * @brief Select if we want to use external sense
         */
        inline static void SetExternalSenseActive(const bool isActive) {
            gShared->isUsingExternalSense = isActive;
            NotifyTask(TaskNotifyBits::UpdateSenseRelay);
        }

    private:
        void main();

        void identifyDriver();

        void readSensors();

    private:
        /// Task handle
        TaskHandle_t task;
        /// Task information structure
        StaticTask_t tcb;
        /// Measurement update timer
        TimerHandle_t sampleTimer;
        /// Storage for sampling timer
        StaticTimer_t sampleTimerBuf;

        /// Last input voltage reading (mV)
        uint32_t inputVoltage{0};
        /// Last input current reading (µA)
        uint32_t inputCurrent{0};
        /// Are we using external voltage sense?
        bool isUsingExternalSense{false};

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

        /**
         * @brief Measurement sample interval (msec)
         *
         * Interval at which the data from the analog board is read.
         */
        constexpr static const size_t kMeasureInterval{10};

        /// Preallocated stack for the task
        StackType_t stack[kStackSize];

        /// Shared task instance
        static Task *gShared;
};

void Start();
}

#endif
