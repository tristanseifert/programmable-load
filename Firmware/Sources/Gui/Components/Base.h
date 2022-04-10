#ifndef GUI_COMPONENTS_BASE_H
#define GUI_COMPONENTS_BASE_H

#include "Gfx/Types.h"

namespace Gfx {
class Framebuffer;
}

/**
 * @brief Namespace containing component implementations
 */
namespace Gui::Components {
/**
 * @brief Base class for GUI components
 *
 * This is an abstract base class for a GUI component. It provides the interface methods called by
 * the GUI code to draw, and dispatch events to a component. Components may also use it to opt in
 * to automatic selection handling.
 */
class Base {
    public:
        Base(const Gfx::Rect bounds) : bounds(bounds) {}
        virtual ~Base() = default;

        /**
         * @brief Draw the component
         *
         * Render the contents of the component on the framebuffer at the location specified by
         * the component's rect.
         */
        virtual void draw(Gfx::Framebuffer &fb) = 0;

    public:
        /**
         * @brief Bounding rectangle of component
         *
         * Defines the origin and size of the component's content.
         */
        Gfx::Rect bounds;
};
}

#endif
