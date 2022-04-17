#ifndef GUI_COMPONENTS_BASE_H
#define GUI_COMPONENTS_BASE_H

#include "../Screen.h"
#include "Divider.h"
#include "StaticIcon.h"
#include "StaticLabel.h"

#include "Gfx/Types.h"
#include "Log/Logger.h"

namespace Gfx {
class Framebuffer;
}

/**
 * @brief Namespace containing component implementations
 */
namespace Gui::Components {
/**
 * @brief Draws a component.
 *
 * Invokes the appropriate draw function for a particular component, given a component data
 * structure.
 */
void Draw(Gfx::Framebuffer &fb, const ComponentData &data) {
    switch(data.type) {
        case ComponentType::Divider:
            Divider::Draw(fb, data);
            break;
        case ComponentType::StaticLabel:
            StaticLabel::Draw(fb, data);
            break;
        case ComponentType::StaticIcon:
            StaticIcon::Draw(fb, data);
            break;

        default:
            Logger::Panic("unknown component type %08x", static_cast<uint32_t>(data.type));
    }
}
}

#endif
