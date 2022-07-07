#ifndef RPC_MESSAGEHANDLER_H
#define RPC_MESSAGEHANDLER_H

#include <stddef.h>
#include <stdint.h>

#include <etl/map.h>
#include <etl/string_view.h>
#include <etl/vector.h>

namespace Rpc {
class Endpoint;

/**
 * @brief RPC work task
 *
 * This encapsulates a task which services interrupts from the mailbox, and thus handles the
 * messages over the virtio interface OpenAMP exposes to the host.
 */
class MessageHandler {
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
             * @brief Mailbox interrupt
             *
             * The mailbox driver received an interrupt and wishes to be serviced.
             */
            MailboxDeferredIrq          = (1 << 0),

            /**
             * @brief System shutdown interrupt
             *
             * Host signalled that we will be shutting down, so gracefully notify all tasks that
             * require it that we're shutting down. We'll also perform some of our own clean-up
             * at this stage.
             */
            ShutdownRequest             = (1 << 1),

            /**
             * @brief All valid notifications
             *
             * Bitwise OR of all notification values defined. These are in turn all bits that are
             * cleared after waiting for a notification.
             */
            All                         = (MailboxDeferredIrq | ShutdownRequest),
        };

        MessageHandler();
        ~MessageHandler();

        int registerEndpoint(const etl::string_view &epName, Endpoint *handler,
                const uint32_t srcAddr = RPMSG_ADDR_ANY,
                const TickType_t timeout = portMAX_DELAY);

    private:
        void main();

    private:
        /**
         * @brief Information about a registered endpoint
         */
        struct EndpointInfo {
            /// remote endpoint info structure
            struct rpmsg_endpoint rpmsgEndpoint;
            /// endpoint class
            Endpoint *handler;
        };

        /// priority of the task
        static const constexpr uint8_t kPriority{Rtos::TaskPriority::Middleware};
        /// Size of the task stack, in words
        static const constexpr size_t kStackSize{420};
        /// Task name (for display purposes)
        static const constexpr etl::string_view kName{"MsgHandler"};
        /// Notification index
        static const constexpr size_t kNotificationIndex{Rtos::TaskNotifyIndex::TaskSpecific};

        /// Maximum number of supported endpoints
        static const constexpr size_t kMaxNumEndpoints{4};
        /// Name for the control endpoint
        static const constexpr etl::string_view kEpNameControl{"pl.control"};

        /// RTOS task handle
        TaskHandle_t handle;
        /// Lock protecting access to OpenAMP resources
        SemaphoreHandle_t lock;

        /// installed endpoint handlers
        etl::map<etl::string_view, EndpointInfo *, kMaxNumEndpoints> endpoints;

        /// message endpoint (control)
        struct rpmsg_endpoint epControl;
};
}

#endif
