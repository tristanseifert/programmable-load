#ifndef RPC_MESSAGEHANDLER_H
#define RPC_MESSAGEHANDLER_H

#include <stddef.h>
#include <stdint.h>

#include <etl/delegate.h>
#include <etl/unordered_map.h>
#include <etl/span.h>
#include <etl/string_view.h>
#include <etl/vector.h>

#include <openamp/open_amp.h>

#include "Rtos/Rtos.h"

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
         * @brief Callback type for a shutdown callback
         *
         * @param mh Message handler task instance that sent the shutdown notification
         * @param ctx Context parameter passed when the callback was registered
         */
        using ShutdownCallback = etl::delegate<void(MessageHandler *mh, void *ctx)>;

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



            /**
             * @brief Shutdown acknowledgement
             *
             * A task has completed its shutdown process after a shutdown notification has been
             * received.
             *
             * @remark This is only observed for during shutdown.
             */
            ShutdownAck                 = (1 << 30),
        };

        MessageHandler();
        ~MessageHandler();

        int registerEndpoint(const etl::string_view &epName, Endpoint *handler,
                const uint32_t srcAddr = RPMSG_ADDR_ANY,
                const TickType_t timeout = portMAX_DELAY);

        void addShutdownHandler(const ShutdownCallback &callback, void *ctx = nullptr);
        void ackShutdown();

        /**
         * @brief Send a packet on the given endpoint
         *
         * Acquires the required locks and then sends a packet on the endpoint specified.
         *
         * @param ep Endpoint to send on
         * @param message Packet to send
         * @param address Host channel address to send to
         * @param timeout How long to wait to acquire the lock
         *
         * @return Positive number of bytes sent, or an error code
         */
        inline int sendTo(struct rpmsg_endpoint *ep, etl::span<const uint8_t> message,
                const uint32_t address, const TickType_t timeout = portMAX_DELAY) {
            BaseType_t ok;
            int err{-1};

            // get lock
            ok = xSemaphoreTakeRecursive(this->lock, timeout);
            if(ok == pdFALSE) {
                return -1;
            }

            // perform the send
            err = rpmsg_sendto(ep, message.data(), message.size(), address);

            // release lock and return
            xSemaphoreGiveRecursive(this->lock);
            return err;
        }

    private:
        void main();

        void handleShutdown();

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

        /**
         * @brief Shutdown callback and auxiliary info
         */
        struct ShutdownCallbackInfo {
            /// function to invoke
            ShutdownCallback callback;
            /// auxiliary context ptr to pass to callback
            void *context{nullptr};
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
        /// Maximum number of shutdown handlers
        static const constexpr size_t kMaxNumShutdownHandlers{8};
        /// Name for the control endpoint
        static const constexpr etl::string_view kEpNameControl{"pl.control"};

        /// RTOS task handle
        TaskHandle_t handle;
        /**
         * @brief Lock protecting access to OpenAMP resources
         *
         * This is a recursive lock so we can use the `send()` helper (and other external methods)
         * from the context of a message callback.
         */
        SemaphoreHandle_t lock;

        /// installed endpoint handlers
        etl::unordered_map<etl::string_view, EndpointInfo *, kMaxNumEndpoints> endpoints;

        /// message endpoint (control)
        struct rpmsg_endpoint epControl;

        /**
         * @brief Shutdown handlers
         *
         * Contains callbacks to invoke when the system receives a shutdown request.
         *
         * @remark Callbacks will be invoked in the order they were registered.
         */
        etl::vector<ShutdownCallbackInfo, kMaxNumShutdownHandlers> shutdownHandlers;

        /**
         * @brief Task shutdown counter
         *
         * This contains the number of tasks that registered for shutdown notifications with
         * acknowledgement. This counter is decremented once each time a task acknowledges the
         * shutdown request, until it either reaches 0 or a timeout expires.
         */
        size_t shutdownCounter{0};
};
}

#endif
