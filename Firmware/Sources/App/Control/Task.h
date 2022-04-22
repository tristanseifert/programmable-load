#ifndef APP_CONTROL_TASK_H
#define APP_CONTROL_TASK_H

#include <stddef.h>
#include <stdint.h>

#include "Rtos/Rtos.h"
#include "Util/Uuid.h"

#include <etl/string_view.h>

#include "LoadDriver.h"

namespace App::Control {
/**
 * @brief Control loop operation mode
 */
enum class OperationMode {
    ConstantCurrent,
    ConstantVoltage,
    ConstantWattage,
};


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
             * @brief Control configuration change
             *
             * Indicate the state of the load control loop has changed, such as the load enable
             * state, operation mode, or setpoints.
             */
            ConfigChange                = (1 << 4),

            /**
             * @brief All valid notify bits
             *
             * Bitwise OR of all notification bits.
             */
            All                         = (ExternalTrigger | IrqAsserted | SampleData |
                    UpdateSenseRelay | ConfigChange),
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
         * @brief Get maximum input voltage
         *
         * @return Maximum allowed input voltage (mV) or -1 if error
         */
        inline static uint32_t GetMaxInputVoltage() {
            uint32_t value{0};

            if(!gShared->driver) {
                return -1;
            }

            const auto err = gShared->driver->getMaxInputVoltage(value);
            if(!err) {
                return value;
            }

            return -1;
        }

        /**
         * @brief Get maximum input current
         *
         * @return Maximum allowed load current (mA) or -1 if error
         */
        inline static uint32_t GetMaxInputCurrent() {
            uint32_t value{0};

            if(!gShared->driver) {
                return -1;
            }

            const auto err = gShared->driver->getMaxInputCurrent(value);
            if(!err) {
                return value;
            }

            return -1;
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

        /**
         * @brief Set the current set point directly
         *
         * This is used externally when the load is in constant current mode.
         *
         * @param current Desired load current, in µA
         */
        inline static void SetCurrentSetpoint(const uint32_t current) {
            gShared->loadCurrentSetpoint = current;
            NotifyTask(TaskNotifyBits::ConfigChange);
        }

        /**
         * @brief Set load state
         *
         * @param isActive Whether the load is active (sinking current)
         */
        inline static void SetIsLoadActive(const bool isActive) {
            gShared->isLoadEnabled = isActive;
            NotifyTask(TaskNotifyBits::ConfigChange);
        }

        /**
         * @brief Determine whether the load is enabled
         */
        inline static auto GetIsLoadActive() {
            return gShared->isLoadEnabled;
        }

        /**
         * @brief Get the current control loop operation mode
         */
        inline static auto GetMode() {
            return gShared->mode;
        }

    private:
        void main();

        void identifyDriver();

        void readSensors();
        void updateConfig();

    private:
        /// Task handle
        TaskHandle_t task;
        /// Task information structure
        StaticTask_t tcb;
        /// Measurement update timer
        TimerHandle_t sampleTimer;
        /// Storage for sampling timer
        StaticTimer_t sampleTimerBuf;

        /// Current control loop mode
        OperationMode mode{OperationMode::ConstantCurrent};
        /// Load set point (µA)
        uint32_t loadCurrentSetpoint{0};

        /// Last input voltage reading (mV)
        uint32_t inputVoltage{0};
        /// Last input current reading (µA)
        uint32_t inputCurrent{0};
        /// Are we using external voltage sense?
        bool isUsingExternalSense{false};
        /// Is the load enabled?
        bool isLoadEnabled{false};
        /// Previous load enable state
        bool prevIsLoadEnabled{false};

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
         * Interval at which the data from the analog board is read. This also sets the speed at
         * which the internal control loop runs and adjusts the output.
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
