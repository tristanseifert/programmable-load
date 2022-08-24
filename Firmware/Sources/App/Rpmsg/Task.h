#ifndef APP_RPMSG_TASK_H
#define APP_RPMSG_TASK_H

#include <stddef.h>
#include <stdint.h>

#include "Rtos/Rtos.h"
#include "Rpc/Endpoints/Handler.h"

#include <etl/array.h>
#include <etl/span.h>
#include <etl/string_view.h>

namespace App::Rpmsg {
/**
 * @brief rpmsg Control Endpoint
 *
 * Handles sending periodic updates of measurement values to the host, as well as processing the
 * more complex requests that aren't handled by the RPC endpoint handler directly.
 */
class Task: public Rpc::Endpoint {
    friend void Start();

    public:
        /**
         * @brief Task notification values
         */
        enum TaskNotifyBits: uint32_t {
            /**
             * @brief Update control data
             *
             * Sends a message to the remote host with updated measurement data.
             *
             * This would be fired by a background timer.
             */
            SendMeasurements            = (1 << 0),

            /**
             * @brief All valid notify bits
             *
             * Bitwise OR of all notification bits.
             */
            All                         = (SendMeasurements),
        };

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

    public:
        Task();
        virtual ~Task();

        void main();

        void handleMessage(etl::span<const uint8_t> message, const uint32_t srcAddr) override;

    private:
        void sendMeasurements();

    private:
        /// Maximum size for a message to be sent, bytes
        constexpr static const size_t kMaxPacketSize{512};

        /// Task handle
        TaskHandle_t task;
        /// Task information structure
        StaticTask_t tcb;
        /// Measurement send update timer
        TimerHandle_t sampleTimer;
        /// Storage for sampling timer
        StaticTimer_t sampleTimerBuf;

        /// Message send buffer
        etl::array<uint8_t, kMaxPacketSize> txBuffer;

    private:
        /**
         * @brief loadd RPC message types
         *
         * @remark These must be kept in sync with the loadd source.
         */
        enum class MsgType: uint8_t {
            /// Do nothing
            NoOp                        = 0x00,
            /**
             * @brief Periodic measurement update
             *
             * This message carries a payload that contains measurement values from the load. This
             * is sent periodically without request from the host.
             */
            Measurement                 = 0x10,
        };


        /// rpmsg channel name
        constexpr static const etl::string_view kRpmsgName{"pl.control"};
        /// rpmsg address
        constexpr static const uint32_t kRpmsgAddress{0x420};

        /// Runtime priority level
        static const constexpr uint8_t kPriority{Rtos::TaskPriority::AppLow};

        /// Size of the task's stack, in words
        static const constexpr size_t kStackSize{420};
        /// Task name (for display purposes)
        static const constexpr etl::string_view kName{"RpmsgRpc"};
        /// Notification index
        static const constexpr size_t kNotificationIndex{Rtos::TaskNotifyIndex::TaskSpecific};

        /**
         * @brief Measurement sample interval (msec)
         *
         * Default reporting interval for updated current, voltage, measurements
         */
        constexpr static const size_t kMeasureInterval{1000};

        /// Preallocated stack for the task
        StackType_t stack[kStackSize];

        /// Shared task instance
        static Task *gShared;
};

void Start();
}

#endif
