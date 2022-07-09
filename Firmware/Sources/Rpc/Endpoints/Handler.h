#ifndef RPC_ENDPOINT_HANDLER_H
#define RPC_ENDPOINT_HANDLER_H

#include <stddef.h>
#include <stdint.h>

#include <etl/span.h>

namespace Rpc {
/**
 * @brief Abstract RPC endpoint class
 *
 * This is an abstract base class used by classes that handle messages on an RPC endpoint.
 */
class Endpoint {
    public:
        virtual ~Endpoint() = default;

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
        virtual void handleMessage(etl::span<const uint8_t> message, const uint32_t srcAddr) = 0;

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
        struct rpmsg_endpoint *ep{nullptr};
};
}

#endif
