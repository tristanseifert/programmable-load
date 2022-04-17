#include "Icon.h"
#include "Framebuffer.h"
#include "Primitives.h"

using namespace Gfx;

/**
 * @brief Draw bitmap on screen
 *
 * Blits the bitmap to the framebuffer at the given position.
 *
 * @param fb Framebuffer to draw on
 * @param origin Top left corner of bitmap
 */
void Icon::draw(Framebuffer &fb, const Point origin) const {
    fb.blit4Bpp(this->bitmap, this->size, origin, Framebuffer::BlitFlags::HasTransparency);
}
