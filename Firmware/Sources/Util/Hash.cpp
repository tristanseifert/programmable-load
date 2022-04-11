#include "Hash.h"

using namespace Util;

#define	FORCE_INLINE inline __attribute__((always_inline))

inline uint32_t rotl32(uint32_t x, int8_t r) {
    return (x << r) | (x >> (32 - r));
}

inline uint64_t rotl64(uint64_t x, int8_t r) {
    return (x << r) | (x >> (64 - r));
}

#define	ROTL32(x,y)	rotl32(x,y)
#define ROTL64(x,y)	rotl64(x,y)

FORCE_INLINE static constexpr uint32_t getblock32(const uint32_t *p, size_t i) {
    return p[i];
}

//-----------------------------------------------------------------------------
// Finalization mix - force all bits of a hash block to avalanche

FORCE_INLINE uint32_t fmix32 (uint32_t h) {
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;

    return h;
}

/**
 * @nrief MurmurHash3
 *
 * MurmurHash3 was written by Austin Appleby, and is placed in the public domain. The author
 * hereby disclaims copyright to this source code.
 */
uint32_t Hash::MurmurHash3(const void *dataIn, const size_t dataLen, const uint32_t seed) {
    const uint8_t * data = static_cast<const uint8_t *>(dataIn);
    const int nblocks = dataLen / 4;

    uint32_t h1 = seed;

    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    //----------
    // body
    const uint32_t * blocks = static_cast<const uint32_t *>(data + nblocks*4);

    for(int i = -nblocks; i; i++) {
        uint32_t k1 = getblock32(blocks, i);

        k1 *= c1;
        k1 = ROTL32(k1, 15);
        k1 *= c2;

        h1 ^= k1;
        h1 = ROTL32(h1, 13);
        h1 = h1*5+0xe6546b64;
    }

    //----------
    // tail
    const uint8_t * tail = static_cast<const uint8_t *>(data + nblocks*4);
    uint32_t k1{0};

    switch(dataLen & 3) {
        case 3: k1 ^= tail[2] << 16;
                [[fallthrough]];
        case 2: k1 ^= tail[1] << 8;
                [[fallthrough]];
        case 1: k1 ^= tail[0];
            k1 *= c1; k1 = ROTL32(k1,15); k1 *= c2; h1 ^= k1;
    };

    //----------
    // finalization
    h1 ^= dataLen;
    h1 = fmix32(h1);
    return h1;
}

