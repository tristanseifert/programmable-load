#ifndef GFX_ICON_H
#define GFX_ICON_H

#include <stddef.h>
#include <stdint.h>

#include <etl/span.h>

#include "Types.h"

namespace Gfx {
class Framebuffer;

struct Icon {
    public:
        /**
         * @brief Size of the icon (in pixels)
         *
         * Defines the final renderable size of the icon
         */
        const Size size;

        /**
         * @brief Bitmap data
         *
         * Actual pixel data (4bpp) for this icon
         */
        etl::span<const uint8_t> bitmap;

    public:
        void draw(Framebuffer &fb, const Point origin) const;

    public:
        /// USB connectivity badge (main screen)
        static const Icon gMainBadgeUsb;
        /// External sense badge (main screen)
        static const Icon gMainBadgeVExt;
};
}

#endif
