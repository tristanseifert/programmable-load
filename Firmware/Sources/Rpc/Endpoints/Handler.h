#ifndef RPC_ENDPOINT_HANDLER_H
#define RPC_ENDPOINT_HANDLER_H

#include <stddef.h>
#include <stdint.h>

#include <etl/span.h>

#include "Log/Logger.h"
#include "Rtos/Rtos.h"

namespace Rpc {
/**
 * @brief Abstract RPC endpoint class
 *
 * This is an abstract base class used by classes that handle messages on an RPC endpoint.
 *
 * It also provides tracking of received message, and blocking on knowing the endpoint on the
 * remote side becoming alive.
 */
class Endpoint {
    public:
        /**
         * @brief Initialize the message handler base class
         */
        Endpoint() {
            this->msgRxSem = xSemaphoreCreateBinary();
            REQUIRE(this->msgRxSem, "%s failed", "xSemaphoreCreateBinary");
        }

        /**
         * @brief Deallocate memory belonging to the base class
         */
        virtual ~Endpoint() {
            vSemaphoreDelete(this->msgRxSem);
        }

        /**
         * @brief Handle a received message
         *
         * Invoked when a message has been received from the host on this endpoint. The message's
         * raw contents are provided, as is the source address which can be used to send a reply.
         *
         * @param message Buffer containing the message
         * @param srcAddr Source address (originator) of the message
         *
         * @remark The message buffer is only valid until this method returns.
         */
        virtual void handleMessage(etl::span<const uint8_t> message, const uint32_t srcAddr) {
            // notify "rx waiting" semaphore if needed (used to pend initial request til we receive)
            if(!__atomic_test_and_set(&this->hasReceivedMsg, __ATOMIC_RELAXED)) {
                xSemaphoreGive(this->msgRxSem);
            }
        }

        /**
         * @brief Remote handler unbound
         *
         * This indicates that the remote endpoint handler unbound from this endpoint, usually
         * because the driver/task responsible for it unloaded.
         */
        virtual void hostDidUnbind() {
            // the default implementation is to do nothing
        }

        /**
         * @brief Endpoint has been created
         *
         * The rpmsg endpoint has been created, and the handler can now invoke methods on it to
         * send messages.
         *
         * @remark You should not try to receive messages through means other than the callbacks
         *         that will automagically be invoked.
         */
        virtual void endpointIsAvailable(struct rpmsg_endpoint *newEp) {
            this->ep = newEp;
        }

    protected:
        /**
         * @brief Wait for the remote endpoint to be available
         *
         * Block the calling task until the remote side of this rpmsg endpoint sends at least one
         * message.
         *
         * @param timeout How long to wait for the remote to become alive
         *
         * @return Whether the remote has become alive or not
         */
        bool waitForRemote(const TickType_t timeout = portMAX_DELAY) {
            // check if we've already received a message
            if(__atomic_load_n(&this->hasReceivedMsg, __ATOMIC_RELAXED)) {
                return true;
            }

            // if not, block on the semaphore
            auto ok = xSemaphoreTake(this->msgRxSem, timeout);
            if(ok != pdTRUE) {
                return false;
            }

            // signal the semaphore, so any subsequent waiting tasks also wake up
            xSemaphoreGive(this->msgRxSem);
            return true;
        }

    protected:
        /// Pointer to the underlying OpenAMP endpoint
        struct rpmsg_endpoint *ep{nullptr};
        /// Semaphore signalled whenever a message is received, addressed to this endpoint
        SemaphoreHandle_t msgRxSem{nullptr};

        /**
         * @brief Flag indicating whether we've received at least one message
         *
         * This is used to know whether the remote endpoint has made itself known, which is
         * required so that we can send unsolicited messages. Otherwise, we don't know what address
         * on the remote end to send them to.
         */
        bool hasReceivedMsg{false};
};
}

#endif
