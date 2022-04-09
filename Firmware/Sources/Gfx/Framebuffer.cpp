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
 *
 * @TODO increase performance by writing two pixels at a time when possible
 */
void Framebuffer::blit4Bpp(etl::span<const uint8_t> source, const Size sourceSize,
        const Point destPoint, const BlitFlags flags) {
    uint8_t srcTemp, dstTemp;
    size_t dstOffset;

    // bail if the entire bitmap will fall outside the framebuffer
    if(destPoint.x >= this->size.width || destPoint.y >= this->size.height) {
        return;
    }

    // calculate X and Y extents for the drawing, and some bitmap info
    const Point endPoint{
        .x = ((destPoint.x + sourceSize.width) > this->size.width) ?
            this->size.width : static_cast<uint16_t>(destPoint.x + sourceSize.width),
        .y = ((destPoint.y + sourceSize.height) > this->size.height) ?
            this->size.height : static_cast<uint16_t>(destPoint.y + sourceSize.height),
    };

    size_t bitmapStride = (sourceSize.width / 2) + (sourceSize.width & 1); // round odd up

    // iterate over each pixel of the framebuffer
    for(uint16_t y = destPoint.y, srcY = 0; y < endPoint.y; y++, srcY++) {
        for(uint16_t x = destPoint.x, srcX = 0; x < endPoint.x; x++, srcX++) {
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

            // read the value of the destination framebuffer
            dstOffset = this->getPixelOffset({x, y});
            dstTemp = this->data[dstOffset];

            if(!(x & 1)) { // even pixel
                dstTemp &= 0x0f;
                dstTemp |= (srcTemp << 4);
            } else { // odd pixel
                dstTemp &= 0xf0;
                dstTemp |= srcTemp;
            }

            this->data[dstOffset] = dstTemp;
        }
    }
}

