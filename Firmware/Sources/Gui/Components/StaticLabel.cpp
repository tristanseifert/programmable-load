#include "StaticLabel.h"

#include "Gfx/Font.h"
#include "Gfx/Framebuffer.h"
#include "Gfx/Primitives.h"

using namespace Gui::Components;

/**
 * @brief Draw the static label
 *
 * We'll first draw the background (if desired) then render the text on top.
 */
void StaticLabel::draw(Gfx::Framebuffer &fb) {
    using namespace Gfx;

    // draw background
    if(this->isInverted) {
        FillRect(fb, this->bounds, 0xf);
    } else {
        FillRect(fb, this->bounds, 0x0);
    }

    // draw text
    // TODO: use fancier API
    this->font->draw(this->string, fb, this->bounds.origin);
}
