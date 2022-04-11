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
     * @brief Context to pass to callbacks
     *
     * An arbitrary pointer-sized value that's passed to all screen callbacks.
     */
    void *callbackContext{nullptr};

    /**
     * @brief Callback invoked immediately before the screen is visible
     *
     * Use this to reset some screen state as the screen is about to be rendered on the screen for
     * the first time.
     */
    void (*willPresent)(Screen *screen, void *context){nullptr};

    /**
     * @brief Callback invoked after the screen is fully visible
     *
     * Invoked after the screen presentation animation, if any, has completed.
     */
    void (*didPresent)(Screen *screen, void *context){nullptr};

    /**
     * @brief Callback invoked immediately before the screen will disappear
     *
     * This is called right before the animation (if any) is started to hide the screen from the
     * display.
     */
    void (*willDisappear)(Screen *screen, void *context){nullptr};

    /**
     * @brief Callback invoked after screen has disappeared
     *
     * Invoked once the screen's disappear animation (if any) has completed.
     */
    void (*didDisappear)(Screen *screen, void *context){nullptr};

    /**
     * @brief Menu button callback
     *
     * Invoked when the menu button is pressed while the controller is visible. This applies only
     * to short button presses: a long press will always open the GUI system's navigation menu to
     * allow exiting from navigation stacks.
     *
     * If this callback is not specified, the default behavior (going up one level in the
     * navigation stack) will apply.
     */
    void (*menuPressed)(Screen *screen, void *context){nullptr};
};
}

#endif
