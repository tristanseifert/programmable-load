#ifndef GUI_COMPONENTS_STATICLABEL_H
#define GUI_COMPONENTS_STATICLABEL_H

#include "../Screen.h"
#include "Gfx/Font.h"

#include <etl/string_view.h>

namespace Gui::Components {
/**
 * @brief Static text label
 *
 * Displays a fixed string in the user interface. It does not support any user interactivity.
 *
 * The text may be drawn 
 */
class StaticLabel {
    public:
        static void Draw(Gfx::Framebuffer &fb, const ComponentData &data);
};
}

#endif
