#ifndef RPC_ENDPOINTS_CONFD_HANDLER_H
#define RPC_ENDPOINTS_CONFD_HANDLER_H

#include <stddef.h>
#include <stdint.h>

#include <etl/delegate.h>
#include <etl/unordered_map.h>
#include <etl/span.h>
#include <etl/string.h>
#include <etl/string_view.h>
#include <etl/variant.h>
#include <etl/vector.h>

#include "Rtos/Rtos.h"
#include "../Handler.h"

namespace Rpc {
class MessageHandler;
}

/// Communication to confd
namespace Rpc::Confd {
/**
 * @brief confd endpoint handler
 *
 * It observes responses to requests we've made, and notifies whatever tasks were waiting for the
 * response.
 */
class Handler: public Rpc::Endpoint {
    friend class Service;

    public:
        Handler();
        ~Handler();

        void attach(MessageHandler *mh);

        void handleMessage(etl::span<const uint8_t> message, const uint32_t srcAddr) override;

    private:
        /**
         * @brief confd RPC message types
         *
         * @remark These must be kept in sync with the confd source.
         */
        enum class MsgType: uint8_t {
            /// Do nothing
            NoOp                        = 0x00,
            /// Access the configuration database (read)
            Query                       = 0x01,
            /// Update the configuration database (write)
            Update                      = 0x02,
        };

        /**
         * @brief Information about a request (used for blocking)
         *
         * This structure holds information on a response to a previously made request to the confd
         * service on the host, and is used in returning data to the caller in that case.
         */
        struct InfoBlock {
            /**
             * @brief Response data for a "query config" (get) request
             *
             * This contains a copy of the response data (so we can access it whenever, rather
             * than causing rpmsg buffer shortages)
             */
            struct GetResponse {
                /// Maximum length of string data
                constexpr static const size_t kMaxStringLen{486};
                /// Maximum length of binary data
                constexpr static const size_t kMaxBlobLen{486};

                /// String type used for result data
                using StringType = etl::string<kMaxStringLen>;
                /// Container type for binary data
                using BlobType = etl::vector<uint8_t, kMaxBlobLen>;

                /// returned key value
                etl::variant<etl::monostate, uint64_t, float, StringType, BlobType> value;

                /// was the key found?
                bool keyFound{false};
            };
            /**
             * @brief Response data for a "update config" (set) request
             */
            struct SetResponse {
                /// was the key successfully updated?
                bool updated{false};
            };

            /// task to unblock on request completion
            TaskHandle_t notificationTask{nullptr};
            /// notification bits to set
            uintptr_t notificationBits{0};

            /// message tag we're waiting for a response on
            uint8_t tag{0};
            /// whether the notification wait has been abandoned (timed out)
            bool abandoned{false};

            /// in case of error, the associated status rpc status code
            int error{0};
            /// response data
            etl::variant<etl::monostate, GetResponse, SetResponse> response;
        };

        using DecoderCallback = etl::delegate<int(etl::span<const uint8_t>, InfoBlock *)>;

        /// rpmsg channel name
        constexpr static const etl::string_view kRpmsgName{"confd"};
        /// rpmsg address
        constexpr static const uint32_t kRpmsgAddress{0x421};
        /// Notification bit (in the driver specific index) to wait on
        constexpr static const uintptr_t kNotifyBit{(1 << 0)};
        /// Maximum number of requests that may be in flight simultaneously
        constexpr static const size_t kMaxInflight{4};

        void handleResponse(etl::span<const uint8_t>, const uint32_t, DecoderCallback);

        int sendRequestAndBlock(etl::span<uint8_t> message, InfoBlock* &outInfoBlock,
                TickType_t timeout = portMAX_DELAY);

        /// Mutex to protect our internal data structures
        SemaphoreHandle_t lock;
        /// mapping of tag -> info blocks
        etl::unordered_map<uint8_t, InfoBlock *, kMaxInflight> requests;

        /// Tag value to use for the next message
        uint8_t nextTag{0};
};
}

#endif
