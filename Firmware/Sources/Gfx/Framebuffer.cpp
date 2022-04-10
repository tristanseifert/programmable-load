#include "Framebuffer.h"

#include "Log/Logger.h"

using namespace Gfx;

/**
 * @brief Blit a 4bpp bitmap into the framebuffer
 *
 * Copy the provided 4bpp bitmap into the framebuffer, with the top left of the bitmap at the
 * specified point.
 *
 * @param source Bitmap data to copy. Assume the bitmap is tightly packed.
 * @param sourceSize Size of the source bitmap, in pixels
 * @param destPoint Top left of the bitmap in the framebuffer
 * @param flags Optional flags controlling the blit
 *
 * @TODO increase performance by writing two pixels at a time when possible
 */
void Framebuffer::blit4Bpp(etl::span<const uint8_t> source, const Size sourceSize,
        const Point destPoint, const BlitFlags flags) {
    uint8_t srcTemp;

    // bail if the entire bitmap will fall outside the framebuffer
    if(destPoint.x >= this->size.width || destPoint.y >= this->size.height) {
        return;
    }

    // calculate X and Y extents for the drawing, and some bitmap info
    const Point endPoint = MakePoint<int>(
        ((destPoint.x + sourceSize.width) > this->size.width) ?
            this->size.width : (destPoint.x + sourceSize.width),
        ((destPoint.y + sourceSize.height) > this->size.height) ?
            this->size.height : (destPoint.y + sourceSize.height)
    );

    size_t bitmapStride = (sourceSize.width / 2) + (sourceSize.width & 1); // round odd up

    // iterate over each pixel of the framebuffer
    for(int16_t y = destPoint.y, srcY = 0; y < endPoint.y; y++, srcY++) {
        for(int16_t x = destPoint.x, srcX = 0; x < endPoint.x; x++, srcX++) {
            // get the value of the source bitmap
            srcTemp = source[(srcY * bitmapStride) + (srcX / 2)];

            if(!(srcX & 1)) { // even pixel
                srcTemp >>= 4;
            } else { // odd pixel
                srcTemp &= 0x0f;
            }

            if((flags & BlitFlags::HasTransparency) && !srcTemp) {
                continue;
            }

            this->setPixel(MakePoint(x, y), srcTemp);
        }
    }
}

/**
 * @brief Blit the contents of a framebuffer to another.
 *
 * Copy the entirety of the provided framebuffer to the destination.
 *
 * @param source Source framebuffer to copy from
 * @param destPoint Top left of the destination in this framebuffer
 * @param flags Optional flags controlling the blit
 */
void Framebuffer::blit4Bpp(const Framebuffer &source, const Point destPoint,
        const BlitFlags flags) {
    return this->blit4Bpp(source.data, source.size, destPoint, flags);
}

