/**
 * @file
 *
 * @brief Common types for graphics subsystem
 */
#ifndef GFX_TYPES_H
#define GFX_TYPES_H

#include <stdint.h>

namespace Gfx {
struct Point {
    int16_t x, y;
};

template<typename IntType = int>
constexpr static inline Point MakePoint(IntType x, IntType y) {
    return {static_cast<int16_t>(x), static_cast<int16_t>(y)};
}

struct Size {
    uint16_t width, height;
};
}

#endif
