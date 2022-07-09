#ifndef RPC_ENDPOINTS_CONFD_SERVICE_H
#define RPC_ENDPOINTS_CONFD_SERVICE_H

#include <stdint.h>
#include <stddef.h>

#include <etl/array.h>
#include <etl/queue.h>
#include <etl/span.h>
#include <etl/string.h>
#include <etl/string_view.h>

#include "Rtos/Rtos.h"
#include "Handler.h"

namespace Rpc {
void Init();
}

namespace Rpc::Confd {
/**
 * @brief User interface to confd RPC channel
 *
 * This class implements function calls that will send queries to the configuration service on the
 * host, via the rpmsg interface. Requests will block the calling task until a response is received
 * or an application-specified timeout expires.
 */
class Service {
    friend class Handler;
    friend void Rpc::Init();

    public:
        /// Status codes
        enum Status: int {
            Success                     = 0,
            Timeout                     = 1,
            KeyNotFound                 = 2,
            ValueTypeMismatch           = 3,
            PermissionDenied            = 4,
            MalformedResponse           = 5,
        };

        int get(const etl::string_view &key, etl::span<uint8_t> outBuffer);
        int get(const etl::string_view &key, etl::istring &outValue);
        int get(const etl::string_view &key, uint64_t &outValue);
        int get(const etl::string_view &key, float &outValue);

    private:
        Service(Handler *handler);
        ~Service();

        void *getPacketBuffer();
        void discardPacketBuffer(void *buffer);

        int serializeQuery(const etl::string_view &key, etl::span<uint8_t> &outPacket);
        static int DeserializeQuery(etl::span<const uint8_t> payload, Handler::InfoBlock *info);

    private:
        /// Maximum number of packet buffers to allocate
        constexpr static const size_t kMaxPacketBuffers{2};
        /// Size of packet buffer, in bytes (affects the maximum size properties to get/set)
        constexpr static const size_t kMaxPacketSize{512};

        /// Message handler (used to send requests)
        Handler *handler;

        /// Lock to protect the buffer cache
        SemaphoreHandle_t cacheLock;
        /// Total number of buffers allocated, ever
        size_t cacheTotal{0};
        /// Buffer cache
        etl::queue<void *, kMaxPacketBuffers, etl::memory_model::MEMORY_MODEL_SMALL> cache;
};
}

#endif
