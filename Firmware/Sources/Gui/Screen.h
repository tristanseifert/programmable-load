#ifndef GUI_SCREEN_H
#define GUI_SCREEN_H

#include <stddef.h>
#include <stdint.h>

#include <etl/delegate.h>
#include <etl/span.h>
#include <etl/string_view.h>

namespace Gui {
namespace Components {
class Base;
}

/**
 * @brief Screen definition
 *
 * The smallest unit used by clients of the GUI library is likely going to be this: a screen, which
 * defines the components on the screen.
 */
struct Screen {
    /**
     * @brief Screen title
     *
     * This is shown when the navigation menu (to go back multiple screens at once) is shown.
     */
    etl::string_view title;

    /**
     * @brief Components on screen
     *
     * A list of all components that should be displayed on this screen. Components will be drawn
     * in the order they are specified here, and likewise, their selection order is defined by
     * this ordering.
     */
    etl::span<Components::Base *> components;

    /**
     * @brief Callback invoked immediately before the screen is visible
     *
     * Use this to reset some screen state as the screen is about to be rendered on the screen for
     * the first time.
     */
    etl::delegate<void(Screen *)> willPresent;
};
}

#endif
