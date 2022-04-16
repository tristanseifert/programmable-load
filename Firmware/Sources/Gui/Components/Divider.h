#ifndef GUI_COMPONENTS_DIVIDER_H
#define GUI_COMPONENTS_DIVIDER_H

#include "../Screen.h"
#include "Gfx/Primitives.h"

namespace Gui::Components {
/**
 * @brief Static divider
 *
 * Dividers are stroked lines that divide areas of the screen.
 */
class Divider {
    public:
        /**
         * @brief Draw the divider
         *
         * We'll draw a one point large line on the leftmost or topmost edge of our bounds.
         */
        static void Draw(Gfx::Framebuffer &fb, const ComponentData &data) {
            Gfx::Point start, end;

            // vertical line
            if(data.bounds.size.width == 1) {
                start = data.bounds.origin;
                end = Gfx::MakePoint<int>(start.x, start.y + data.bounds.size.height);
            }
            // vertical line
            else {
                start = data.bounds.origin;
                end = Gfx::MakePoint<int>(start.x + data.bounds.size.width, start.y);
            }

            // draw the line
            Gfx::DrawLine(fb, start, end, data.divider.color);
        }
};
}

#endif
