#include "StaticLabel.h"

#include "Gfx/Font.h"
#include "Gfx/Framebuffer.h"
#include "Gfx/Primitives.h"

#include "Log/Logger.h"

using namespace Gui::Components;

/**
 * @brief Draw the static label
 *
 * We'll first draw the background (if desired) then render the text on top.
 */
void StaticLabel::Draw(Gfx::Framebuffer &fb, const ComponentData &data) {
    using namespace Gfx;

    const auto &info = data.staticLabel;

    // draw background
    if(data.isInverted) {
        FillRect(fb, data.bounds, 0xf);
    } else {
        FillRect(fb, data.bounds, 0x0);
    }

    // draw text
    info.font->draw(info.string, fb, data.bounds, info.fontMode);
}
