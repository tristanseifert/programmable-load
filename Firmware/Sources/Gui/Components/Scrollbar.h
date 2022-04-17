#ifndef GUI_COMPONENTS_SCROLLBAR_H
#define GUI_COMPONENTS_SCROLLBAR_H

#include "Gfx/Primitives.h"
#include "Log/Logger.h"

namespace Gui::Components {
/**
 * @brief Draws a vertical or horizontal scroll bar
 *
 * Scroll bars consist of a filled track, and a knob on that track that indicates the relative
 * position in the represented content.
 */
class Scrollbar {
    public:
        /**
         * @brief Draw the scroll bar
         *
         * @param fb Framebuffer to draw in to
         * @param bounds Rectangle enclosing the entirety of the scroll bar
         * @param position Offset into the total number of items (used for knob position)
         * @param total Total number of items (used for knob size). If this is zero, no knob will
         *        be drawn on the scrollbar.
         */
        static void Draw(Gfx::Framebuffer &fb, const Gfx::Rect bounds, const size_t position,
                const size_t total) {
            // vertical bar?
            if(bounds.size.height > bounds.size.width) {
                DrawVertical(fb, bounds, position, total);
            } 
            // horizontal bar (not yet supported)
            else {
                Logger::Panic("Horizontal scrollbar not implemented");
            }
        }

        /// Width of a vertical scroll bar
        constexpr static const size_t kBarWidth{10};
        /// Minimum height for a vertical knob
        constexpr static const size_t kMinKnobSizeY{4};

        /// Color for the divider
        constexpr static const uint32_t kDividerColor{0x9};
        /// Color for the track background
        constexpr static const uint32_t kTrackBackground{0x1};
        /// Background color of the scroll knob
        constexpr static const uint32_t kKnobBackground{0xd};

    private:
        /**
         * @brief Draw a vertical scrollbar
         */
        static inline void DrawVertical(Gfx::Framebuffer &fb, const Gfx::Rect bounds,
                const size_t position, const size_t total) {
            // inset the track to the left one (for the divider line)
            auto trackBounds = bounds;
            trackBounds.size.width -= 1;
            trackBounds.origin.x += 1;

            // draw the divider and track background
            Gfx::DrawLine(fb, bounds.origin, Gfx::MakePoint<int>(bounds.origin.x,
                        bounds.origin.y + bounds.size.height), kDividerColor);
            Gfx::FillRect(fb, trackBounds, kTrackBackground);

            if(!total) {
                return;
            }

            // calculate the knob height
            auto knobHeight = bounds.size.height / total;
            if(knobHeight < kMinKnobSizeY) {
                knobHeight = kMinKnobSizeY;
            }

            // calculate knob's position and draw
            const auto knobRange = trackBounds.size.height - knobHeight;
            const uint16_t yOffset = static_cast<float>(knobRange) *
                (static_cast<float>(position) / static_cast<float>(total));

            const Gfx::Rect knobBounds{
                Gfx::MakePoint<int>(trackBounds.origin.x,
                        trackBounds.origin.y + yOffset),
                Gfx::MakeSize<int>(trackBounds.size.width, knobHeight),
            };

            Gfx::FillRect(fb, knobBounds, kKnobBackground);
        }
};
}

#endif
