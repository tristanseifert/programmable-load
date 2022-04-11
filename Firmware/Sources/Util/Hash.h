#ifndef UTIL_HASH_H
#define UTIL_HASH_H

#include <stddef.h>
#include <stdint.h>

#include <etl/span.h>

namespace Util {
/**
 * @brief Non-cryptographic hash functions
 *
 * A variety of hash functions (NOT for cryptographic use!)
 */
class Hash {
    public:
        inline static uint32_t MurmurHash3(etl::span<const uint8_t> data, const uint32_t seed = 0) {
            return MurmurHash3(data.data(), data.size(), seed);
        }
        static uint32_t MurmurHash3(const void *data, const size_t dataLen,
                const uint32_t seed = 0);
};
}

#endif
