/**
 * @file
 *
 * @brief Common types for graphics subsystem
 */
#ifndef GFX_TYPES_H
#define GFX_TYPES_H

#include <stdint.h>

namespace Gfx {
/**
 * @brief A single point on screen
 *
 * Coordinates are assumed to have a top-left origin, then increasing as you go down/right.
 */
struct Point {
    int16_t x, y;
};

template<typename IntType = int>
constexpr static inline Point MakePoint(IntType x, IntType y) {
    return {static_cast<int16_t>(x), static_cast<int16_t>(y)};
}

/**
 * @brief Two dimensional size
 */
struct Size {
    uint16_t width, height;
};

template<typename IntType = int>
constexpr static inline Size MakeSize(IntType w, IntType h) {
    return {static_cast<uint16_t>(w), static_cast<uint16_t>(h)};
}

/**
 * @brief A rectangle
 *
 * Combination of origin and size
 */
struct Rect {
    Point origin;
    Size size;

    /**
     * @brief Return a rectangle inset by the given number of pixels.
     */
    constexpr inline Rect inset(const int16_t pixels) const {
        return {
            MakePoint<int>(origin.x + pixels, origin.y + pixels),
            MakeSize<int>(this->size.width - pixels, this->size.height - pixels),
        };
    }
};
}

#endif
