#ifndef RPC_ENDPOINTS_RESOURCEMANAGER_HANDLER_H
#define RPC_ENDPOINTS_RESOURCEMANAGER_HANDLER_H

#include <stddef.h>
#include <stdint.h>

#include <etl/span.h>
#include <etl/string_view.h>
#include <etl/vector.h>

#include "Rtos/Rtos.h"
#include "../Handler.h"

namespace Rpc {
class MessageHandler;
}

/// Linux rpmsg resource manager interface
namespace Rpc::ResourceManager {
/**
 * @brief Resource manager endpoint handler
 *
 * Handles messages exchanged with the resource manager kernel driver
 */
class Handler: public Rpc::Endpoint {
    friend class Service;

    public:
        Handler();
        ~Handler();

        void attach(MessageHandler *mh);

        void handleMessage(etl::span<const uint8_t> message, const uint32_t srcAddr) override;

    private:
        int sendRequestAndBlock(etl::span<uint8_t> message, etl::span<uint8_t> &outRawResponse,
                TickType_t timeout = portMAX_DELAY);

    private:
        /// rpmsg channel name
        constexpr static const etl::string_view kRpmsgName{"rproc-srm"};
        /// rpmsg address
        constexpr static const uint32_t kRpmsgAddress{RPMSG_ADDR_ANY};
        /// Notification bit (in the driver specific index) to wait on
        constexpr static const uintptr_t kNotifyBit{(1 << 1)};
        /// Maximum message length (bytes)
        constexpr static const size_t kMaxMessageLen{128};

        /// Task signalled when an RPC message is received
        TaskHandle_t waitingTask{nullptr};

        /// Buffer to store received messages
        etl::vector<uint8_t, kMaxMessageLen> rxBuffer;
};
}

#endif
