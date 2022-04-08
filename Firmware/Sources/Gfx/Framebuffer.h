#ifndef GFX_FRAMEBUFFER_H
#define GFX_FRAMEBUFFER_H

#include <stddef.h>
#include <stdint.h>

#include <etl/span.h>
#include <etl/utility.h>

#include "Types.h"

namespace Gfx {
/**
 * @brief Description of framebuffer memory
 *
 * Provides the dimensions and stride needed to access a piece of memory representing a
 * framebuffer.
 *
 * @remark Currently, only 4bpp framebuffers are supported.
 */
class Framebuffer {
    public:
        /// Framebuffer format
        enum class Format: uint8_t {
            /// Greyscale, 4 bits per pixel
            Grey4                       = 4,
        };

    public:
        /**
         * @brief Pixel format
         *
         * Format of the pixels stored in the framebuffer
         */
        Format format{Format::Grey4};

        /**
         * @brief Dimensions (width x height)
         */
        Size size;

        /**
         * @brief Framebuffer data
         *
         * A piece of memory that contains the actual pixel data of the framebuffer.
         */
        etl::span<uint8_t> data;

        /**
         * @brief Bytes per line
         *
         * Number of bytes in framebuffer to go from one row to the next.
         */
        size_t stride;

    public:
        void blit4Bpp(etl::span<const uint8_t> source, const Size sourceSize,
                const Point destPoint);

        /**
         * @brief Calculate pixel offset into framebuffer
         *
         * @param point Point in the framebuffer to get the memory address for
         */
        inline constexpr size_t getPixelOffset(const Point point) {
            // TODO: handle other color formats
            return (point.y * this->stride) + (point.x / 2);
        }

    public:
        /**
         * @brief Primary system framebuffer
         *
         * This framebuffer corresponds to the in-memory buffer that'll be transferred to the
         * display when it's dirtied.
         */
        static Framebuffer gMainBuffer;
};
}

#endif
