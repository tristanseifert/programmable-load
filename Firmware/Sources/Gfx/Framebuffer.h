#ifndef GFX_FRAMEBUFFER_H
#define GFX_FRAMEBUFFER_H

#include <stddef.h>
#include <stdint.h>

#include <etl/algorithm.h>
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
        /**
         * @brief Flags for bit blits
         *
         * These flags can be combined via bitwise OR, unless otherwise specified.
         */
        enum BlitFlags: uint32_t {
            None                        = 0,

            /**
             * @brief Source has transparency
             *
             * If set, the source bitmap is considered to contain transparency: for bitmaps without
             * an explicit alpha channel, a pixel value of 0 is considered transparent.
             */
            HasTransparency             = (1 << 0),
        };

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
                const Point destPoint, const BlitFlags flags = BlitFlags::None);
        void blit4Bpp(const Framebuffer &source, const Point destPoint,
                const BlitFlags flags = BlitFlags::None);

        /**
         * @brief Clear the framebuffer
         *
         * The framebuffer will be filled with zero bytes.
         */
        void clear() {
            etl::fill(this->data.begin(), this->data.end(), 0);
        }

        /**
         * @brief Calculate pixel offset into framebuffer
         *
         * @param point Point in the framebuffer to get the memory address for
         */
        inline constexpr size_t getPixelOffset(const Point point) {
            // TODO: handle other color formats
            return (point.y * this->stride) + (point.x / 2);
        }

        /**
         * @brief Set pixel value
         *
         * Sets the value for a particular pixel, taking into account the underlying pixel format.
         *
         * @param point Pixel location to set
         * @param value Value to set for this pixel
         *
         * @TODO Handle non-4bpp pixel formats
         */
        inline void setPixel(const Point point, const uint32_t value) {
            const auto offset = this->getPixelOffset(point);
            if(offset >= this->data.size()) return;

            auto temp = this->data[offset];

            if(!(point.x & 1)) { // even pixel
                temp &= 0x0f;
                temp |= ((value & 0x0f) << 4);
            } else { // odd pixel
                temp &= 0xf0;
                temp |= value & 0x0f;
            }

            this->data[offset] = temp;
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
