#ifndef APP_PINBALL_TASK_H
#define APP_PINBALL_TASK_H

#include <stddef.h>
#include <stdint.h>

#include "FrontIoDriver.h"
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
             * @brief Update UI
             *
             * The user interface needs to be redrawn. This is usually set by the user interface
             * layer as long as it's doing animations, or when user interaction causes the display
             * to be dirtied.
             */
            RedrawUI                    = (1 << 4),

            /**
             * @brief Present main screen
             *
             * Reset the UI to show only the instrument home screen.
             */
            ShowHomeScreen              = (1 << 5),

            /**
             * @brief Process GUI work queue
             *
             * The GUI work queue needs to be processed.
             */
            ProcessWorkQueue            = (1 << 6),

            /**
             * @brief Process the beeper melody
             *
             * Call into the handler for the beeper to process the next event in the currently
             * playing melody.
             */
            ProcessMelody               = (1 << 7),

            /**
             * @brief Update front panel indicators
             *
             * Requests the indicators (including illuminated buttons) on the front panel are
             * updated to the latest state.
             */
            UpdateIndicators            = (1 << 8),

            /**
             * @brief All valid notify bits
             *
             * Bitwise OR of all notification bits.
             */
            All                         = (FrontIrq | RearIrq | PowerPressed | EncoderChanged |
                    RedrawUI | ShowHomeScreen | ProcessWorkQueue | ProcessMelody |
                    UpdateIndicators),
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

        /**
         * @brief Send a notification
         *
         * Notify the UI task something happened.
         *
         * @brief bits Notification bits to set
         */
        static void NotifyTask(const TaskNotifyBits bits) {
            xTaskNotifyIndexed(gShared->task, kNotificationIndex, bits, eSetBits);
        }

        /**
         * @brief Handle front panel button changes
         *
         * Invoked from the driver's IRQ handler (which runs inside our task main loop) when the
         * control buttons state changes.
         *
         * @param down Buttons pressed down
         * @param up Buttons released
         */
        static inline void ButtonsChanged(const FrontIoDriver::Button down,
                const FrontIoDriver::Button up) {
            gShared->handleButtons(down, up);
        }

    private:
        Task();

        void main();
        void detectFrontPanel();
        void showVersionScreen();

        void doChristmasTreeTest();
        void updateIndicators();
        void handleButtons(const FrontIoDriver::Button down, const FrontIoDriver::Button up);

    private:
        /// Task handle
        TaskHandle_t task;
        /// Periodic redraw timer
        TimerHandle_t redrawTimer;

        /// Front IO board driver instance
        FrontIoDriver *frontDriver{nullptr};
        /// Hardware revision of front panel board
        uint16_t frontRev{0};
        /// Front panel driver id
        Util::Uuid frontDriverId;

        /// Timer to dismiss the version screen
        TimerHandle_t versionDismissTimer;

        /**
         * @brief We just disabled the load
         *
         * Set when the "load on" button is pushed down. This ensures we don't re-enable it
         * after the button is released.
         */
        bool didDisableLoad{false};

    private:
        /// Runtime priority level
        static const constexpr uint8_t kPriority{Rtos::TaskPriority::AppLow};

        /// Size of the task's stack, in words
        static const constexpr size_t kStackSize{420};
        /// Task name (for display purposes)
        static const constexpr etl::string_view kName{"Pinball"};
        /// Notification index
        static const constexpr size_t kNotificationIndex{Rtos::TaskNotifyIndex::TaskSpecific};

        /// Duration to show the version/information screen, in milliseconds
        static const constexpr size_t kShowVersionDuration{5*1000};

        /**
         * @brief Interval of forced redraws, in milliseconds
         *
         * Sets the interval of a timer which forces the display to be redrawn. This guards against
         * situations where some display controllers might corrupt the graphics memory if sitting
         * idle for a long time.
         *
         * This also ensures that the task gets periodic events, even with no user or display
         * activity, which in turn ensures the watchdog is kicked.
         *
         * @remark This timer is reset any time the screen is redrawn. This means we won't insert
         *         spurious redraws in the middle of animations or UI, for example.
         */
        constexpr static const size_t kRedrawTimerInterval{800};


        /// Task information structure
        StaticTask_t tcb;
        /// Preallocated stack for the task
        StackType_t stack[kStackSize];

        /// Redraw timer
        StaticTimer_t redrawTimerStorage;

        /// Shared task instance
        static Task *gShared;
};

void Start();
}

#endif
