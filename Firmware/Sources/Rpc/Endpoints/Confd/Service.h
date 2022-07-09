#ifndef RPC_ENDPOINTS_CONFD_SERVICE_H
#define RPC_ENDPOINTS_CONFD_SERVICE_H

#include <stdint.h>
#include <stddef.h>

#include <etl/queue.h>
#include <etl/span.h>
#include <etl/string.h>
#include <etl/string_view.h>
#include <etl/variant.h>

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
            IsNull                      = 6,
        };

        int get(const etl::string_view &key, etl::span<uint8_t> outBuffer, size_t &outNumBytes);
        inline int get(const etl::string_view &key, etl::span<uint8_t> outBuffer) {
            size_t temp;
            return this->get(key, outBuffer, temp);
        }
        int get(const etl::string_view &key, etl::istring &outValue);
        int get(const etl::string_view &key, uint64_t &outValue);
        int get(const etl::string_view &key, float &outValue);

        /**
         * @brief Set a blob configuration value
         *
         * @param key Name of the key to set
         * @param value Blob value to set
         */
        int set(const etl::string_view &key, const etl::span<uint8_t> value) {
            Handler::InfoBlock *block{nullptr};
            bool updated{false};

            int err = this->setCommon(key, value, block, updated);
            if(err) {
                return err;
            }

            delete block;
            return updated ? Status::Success : Status::PermissionDenied;
        }
        /**
         * @brief Set a string configuration value
         *
         * @param key Name of the key to set
         * @param value String value to set
         */
        int set(const etl::string_view &key, const etl::string_view &value) {
            Handler::InfoBlock *block{nullptr};
            bool updated{false};

            int err = this->setCommon(key, value, block, updated);
            if(err) {
                return err;
            }

            delete block;
            return updated ? Status::Success : Status::PermissionDenied;
        }
        /**
         * @brief Set an integer configuration value
         *
         * @param key Name of the key to set
         * @param value Integer value to set
         */
        int set(const etl::string_view &key, const uint64_t value) {
            Handler::InfoBlock *block{nullptr};
            bool updated{false};

            int err = this->setCommon(key, value, block, updated);
            if(err) {
                return err;
            }

            delete block;
            return updated ? Status::Success : Status::PermissionDenied;
        }
        /**
         * @brief Set an floating point configuration value
         *
         * @param key Name of the key to set
         * @param value Float value to set
         */
        int set(const etl::string_view &key, const float value) {
            Handler::InfoBlock *block{nullptr};
            bool updated{false};

            int err = this->setCommon(key, value, block, updated);
            if(err) {
                return err;
            }

            delete block;
            return updated ? Status::Success : Status::PermissionDenied;
        }

    private:
        using ValueType = etl::variant<etl::monostate, etl::span<uint8_t>, etl::string_view,
              uint64_t, float>;

        Service(Handler *handler);
        ~Service();

        void *getPacketBuffer();
        void discardPacketBuffer(void *buffer);

        int getCommon(const etl::string_view &key, Handler::InfoBlock* &outBlock, bool &outFound);
        int serializeQuery(const etl::string_view &key, etl::span<uint8_t> &outPacket);
        static int DeserializeQuery(etl::span<const uint8_t> payload, Handler::InfoBlock *info);

        int setCommon(const etl::string_view &key, const ValueType &newValue,
                Handler::InfoBlock* &outBlock, bool &outUpdated);
        int serializeUpdate(const etl::string_view &key, const ValueType &newValue,
                etl::span<uint8_t> &outPacket);
        static int DeserializeUpdate(etl::span<const uint8_t> payload, Handler::InfoBlock *info);

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
