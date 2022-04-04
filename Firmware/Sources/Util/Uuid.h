#ifndef UTIL_UUID_H
#define UTIL_UUID_H

#include "Log/Logger.h"

#include <stdint.h>
#include <string.h>

#include <etl/array.h>
#include <etl/span.h>
#include <printf/printf.h>

namespace Util {
/**
 * @brief Wrapper for an UUID
 *
 * This class represents a standard 16-byte UUID. It provides helpers for formatting it to strings.
 */
class Uuid {
    public:
        /// Size of an UUID in bytes
        constexpr const static size_t kByteSize{16};

        /**
         * @brief Default initialize an UUID to all zeroes.
         */
        Uuid() = default;

        /**
         * @brief Initialize an UUID from a given blob
         *
         * @param buf Buffer containing the uuid
         */
        Uuid(etl::span<const uint8_t> buf) {
            REQUIRE(buf.size() >= kByteSize, "uuid buffer too small (%zu)", buf.size());
            memcpy(this->data.data(), buf.data(), kByteSize);
        }

        auto operator==(const Uuid &second) const {
            return !memcmp(second.data.data(), this->data.data(), kByteSize);
        }

        /**
         * @brief Format UUID as string
         *
         * @param str String buffer to receive the data
         */
        void format(etl::span<char> str) {
            snprintf(str.data(), str.size(),
                    "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                    data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
                    data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);
        }

    private:
        /**
         * @brief UUID data storage
         */
        etl::array<uint8_t, kByteSize> data;
};
}

#endif
