#ifndef GUI_SCREEN_H
#define GUI_SCREEN_H

#include <stddef.h>
#include <stdint.h>

#include <etl/delegate.h>
#include <etl/span.h>
#include <etl/string_view.h>

#include "Gfx/Types.h"
#include "Gfx/Font.h"
#include "Gfx/Icon.h"

namespace Gui {
namespace Components {
class Base;
struct ListState;
struct NumericSpinnerState;
}
/**
 * @brief Component type value
 *
 * These serve as an index into the GUI system's table of drawing and event handling routines.
 */
enum class ComponentType: uint32_t {
    /**
     * @brief Null entry
     *
     * This is used to indicate that we've reached the end of a list; all processing for
     * components will stop when this type is encountered.
     */
    None                                = 0,

    /**
     * @brief Static text label
     *
     * A simple text label that displays a string.
     */
    StaticLabel                         = 1,

    /**
     * @brief Content divider
     *
     * A vertical or horizontal line
     */
    Divider                             = 2,

    /**
     * @brief Static image/icon
     *
     * Renders a single static icon (a thin wrapper around a raw bitmap) at a fixed location on
     * the screen.
     */
    StaticIcon                          = 3,

    /**
     * @brief Table/list view
     *
     * Presents a scrollable list of items. Callbacks are used to determine the number of items,
     * and to draw each item.
     */
    List                                = 4,

    /**
     * @brief Numeric value adjuster
     *
     * A box that allows entering whole numbers (by incrementing them up or down) with the encoder
     * wheel when selected.
     */
    NumericSpinner                      = 5,
};

/**
 * @brief Component definition
 *
 * Defines the static payload needed to render a particular component.
 */
struct ComponentData {
    /**
     * @brief Type of component
     *
     * This indicates which of the payload fields is valid, and in turn, which drawing routine
     * will be invoked.
     */
    ComponentType type;

    /**
     * @brief Component bounds
     *
     * The bounding rectangle inside which the component renders its contents.
     */
    Gfx::Rect bounds;

    /// Payload holder
    union {
        /// Content divider
        struct {
            uint32_t color{0};
        } divider;
        /// Static text label
        struct {
            /// UTF-8 encoded string to display
            const char *string{nullptr};
            /// Font to display the string in
            const Gfx::Font *font{nullptr};
            /// Render flags for the font
            Gfx::FontRenderFlags fontMode;
        } staticLabel;
        /// Static icon
        struct {
            /// The icon to draw
            const Gfx::Icon *icon{nullptr};
            /// Set to fill the area of the icon with transparent instead
            bool hideIcon;
        } staticIcon;
        /// List view
        struct {
            /// List state buffer
            Components::ListState *state{nullptr};
            /// Height of a row, in pixels
            uint16_t rowHeight{21};

            /// Context for callbacks
            void *context{nullptr};
            /**
             * @brief Callback to retrieve number of rows in list
             *
             * This should return the total number of rows to be displayed in this list.
             *
             * @param context The value in the `context` variable in this struct
             */
            size_t (*getNumRows)(void *context);
            /**
             * @brief Callback to draw a row
             *
             * Invoked to draw the row at the specified index, in the bounds specified.
             *
             * @param fb Framebuffer to draw the row into
             * @param bounds Where on screen to draw the row
             * @param index Row index to draw
             * @param isSelected Whether the row is selected
             * @param context The value of the `context` variable in this struct
             */
            void (*drawRow)(Gfx::Framebuffer &fb, const Gfx::Rect bounds, const size_t index,
                    const bool isSelected, void *context);
            /**
             * @brief Callback invoked when row is selected
             *
             * @param index Row index that was selected
             * @param context The value of the `context` variable
             */
            void (*rowSelected)(const size_t index, void *context);
        } list;
        /// Number spinner
        struct {
            /// State structure
            Components::NumericSpinnerState *state{nullptr};

            /// Font to display the value in
            const Gfx::Font *font{nullptr};
            /// Render flags for the font
            Gfx::FontRenderFlags fontMode;

            /**
             * @brief Display units
             *
             * Defines a transformation from the raw numerical value of the spinner to a display
             * string. In effect, this defines a divisor (to transform the integer value to a
             * floating point value) and the number of digits before and after the decimal point
             * to display, as well as an unit string.
             *
             * Each unit is accompanied with a lower bound, which is the _absolute_ value of the
             * integer value of the component above which this unit is selected. When formatting
             * for display, we'll select the last unit in this list with a lower bound greater than
             * or equal to the current value.
             *
             * @remark Terminate the list with a `NULL` value.
             */
            struct unit {
                /// Absolute lower bound
                uint32_t lowerBound{0};
                /// Divisor
                float divisor{0.f};
                /// Unit display name (or `NULL` for none)
                const char *displayName{nullptr};

                /// Number of digits to the left of the decimal point to show
                uint8_t leftDigits:4{3};
                /// Number of digits to the right of the decimal point to show
                uint8_t rightDigits:4{0};
            };
            const struct unit **units;

            /// Context for callbacks
            void *context{nullptr};
            /**
             * @brief Callback invoked when value changes
             *
             * @param value Current spinner value
             * @param context Value of the `context` variable
             */
            void (*valueChanged)(const int32_t value, void *context);
        } numSpinner;
    };

    /// Is the control hidden?
    uint8_t isHidden:1{0};
    /// Is the control drawn inverted?
    uint8_t isInverted:1{0};
};

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

    /// Total number of components to draw
    const size_t numComponents{0};
    /**
     * @brief Components on screen
     *
     * A list of all components that should be displayed on this screen. Components will be drawn
     * in the order they are specified here, and likewise, their selection order is defined by
     * this ordering.
     */
    const ComponentData *components;

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
    void (*willPresent)(const Screen *screen, void *context){nullptr};

    /**
     * @brief Callback invoked after the screen is fully visible
     *
     * Invoked after the screen presentation animation, if any, has completed.
     */
    void (*didPresent)(const Screen *screen, void *context){nullptr};

    /**
     * @brief Callback invoked immediately before the screen will disappear
     *
     * This is called right before the animation (if any) is started to hide the screen from the
     * display.
     */
    void (*willDisappear)(const Screen *screen, void *context){nullptr};

    /**
     * @brief Callback invoked after screen has disappeared
     *
     * Invoked once the screen's disappear animation (if any) has completed.
     */
    void (*didDisappear)(const Screen *screen, void *context){nullptr};

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
    void (*menuPressed)(const Screen *screen, void *context){nullptr};

    /**
     * @brief Pre-draw callback
     *
     * Invoked immediately before the screen is rendered. The screen can use this callback to
     * update the state of the user interface.
     */
    void (*willDraw)(const Screen *screen, void *context){nullptr};
};
}

#endif
