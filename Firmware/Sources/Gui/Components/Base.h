#ifndef GUI_COMPONENTS_BASE_H
#define GUI_COMPONENTS_BASE_H

#include "Types.h"
#include "../Screen.h"

#include "Gfx/Types.h"
#include "Log/Logger.h"

#include "Divider.h"
#include "List.h"
#include "NumericSpinner.h"
#include "StaticIcon.h"
#include "StaticLabel.h"

namespace Gfx {
class Framebuffer;
}

namespace Gui::Components {
/**
 * @brief Draws a component.
 *
 * Invokes the appropriate draw function for a particular component, given a component data
 * structure.
 *
 * @param flags Modifiers for the current drawing mode
 */
inline void Draw(Gfx::Framebuffer &fb, const ComponentData &data, const DrawFlags flags) {
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
        case ComponentType::List:
            List::Draw(fb, data);
            break;
        case ComponentType::NumericSpinner:
            NumericSpinner::Draw(fb, data, flags);
            break;

        default:
            Logger::Panic("unknown component type %08x", static_cast<uint32_t>(data.type));
    }
}

/**
 * @brief Test if a component is selectable
 *
 * @param component Reference to component data
 */
constexpr inline bool IsSelectable(const ComponentData &data) {
    switch(data.type) {
        case ComponentType::List:
            return true;
        case ComponentType::NumericSpinner:
            return true;

        default:
            return false;
    }
}

/**
 * @brief Handle a selection event for a control.
 *
 * This will dispatch the event to the appropriate method for the control type.
 *
 * @param screen Screen on which the control is displayed
 * @param data Component data
 *
 * @return Whether the component desires all input events; that is, if it returns `true` here,
 *         encoder events will be sent to the control rather than navigation.
 */
constexpr inline bool HandleSelection(const Screen *screen, const ComponentData &data) {
    switch(data.type) {
        case ComponentType::List:
            // you can't get out of a list
            List::HandleSelection(data);
            return true;
        case ComponentType::NumericSpinner:
            return NumericSpinner::HandleSelection(data);

        // other controls ignore selection event
        default:
            return false;
    }
}

/**
 * @brief Handle an encoder event for a control
 *
 * Dispatches the encoder event (rotation delta) to the control's handler, based on its type; this
 * will only be invoked if the control is currently selected (e.g. move mode is inactive)
 *
 * @param screen Screen on which the control is displayed
 * @param data Component data
 * @param delta Encoder movement delta
 * @param needsDraw Variable set if the screen should be redrawn
 */
constexpr inline void HandleEncoder(const Screen *screen, const ComponentData &data,
        const int delta, bool &needsDraw) {
    switch(data.type) {
        // scroll list
        case ComponentType::List:
            List::HandleEncoder(data, delta, needsDraw);
            break;
        // change input value
        case ComponentType::NumericSpinner:
            NumericSpinner::HandleEncoder(data, delta, needsDraw);
            break;

        default:
            break;
    }
}
}

#endif
