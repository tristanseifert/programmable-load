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
    uint16_t x, y;
};

struct Size {
    uint16_t width, height;
};
}

#endif
