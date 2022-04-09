/**
 * @file
 *
 * @brief Graphic primitives
 *
 * Implementations of a few low level graphics drawing primitives
 */
#ifndef GFX_PRIMITIVES_H
#define GFX_PRIMITIVES_H

#include <stddef.h>
#include <stdint.h>

#include "Types.h"

namespace Gfx {
class Framebuffer;

void DrawLine(Framebuffer &fb, const Point start, const Point end, const uint32_t color);
}

#endif
