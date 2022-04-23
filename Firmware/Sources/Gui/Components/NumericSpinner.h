#ifndef GUI_COMPONENTS_NUMERICSPINNER_H
#define GUI_COMPONENTS_NUMERICSPINNER_H

#include "Types.h"
#include "../Screen.h"
#include "Gfx/Primitives.h"

#include <printf/printf.h>

namespace Gui::Components {
/**
 * @brief Spinner state
 *
 * State of the spinner control, including its value and step size.
 */
struct NumericSpinnerState {
    /// Is step mode enabled?
    uint32_t stepModeEnabled:1{0};
    /// Current offset into step list (if any)
    uint32_t stepOffset:4{0};
    /// Offset into unit list
    uint32_t unitOffset:4{0};
    /// Is number input limited? (If clear, the minimum/maximum fields are ignored)
    uint32_t isLimited:1{0};
    /// Are we currently active?
    uint32_t isActive:1{0};
    /// Has the numerical value changed since the last draw?
    uint32_t valueDirty:1{1};

    /// Minimum value
    int32_t minimum{0};
    /// Maxumum value
    int32_t maximum{0};
    /// Current value
    int32_t value{0};

    /// value display string
    char displayBuf[16];
};

/**
 * @brief Number input box
 *
 * Allows the user to adjust a number.
 *
 * Numbers are presented with an associated unit. If desired, multiple unit ranges can be
 * specified, to allow the displayed value to scale with the underlying number. This affects
 * only the presentation on screen.
 *
 * Additionally, the control can opt in to one or more "ranges" which can be cycled between
 * when pressing down the encoder.
 */
class NumericSpinner {
    public:
        /**
         * @brief Draw the list view
         */
        static void Draw(Gfx::Framebuffer &fb, const ComponentData &data, const DrawFlags flags) {
            // validate data
            auto &d = data.numSpinner;
            REQUIRE(d.state, "missing spinner state");

            // outline and fill
            const bool isSelected = TestFlags(flags & DrawFlags::Selected);
            const auto contentBounds = data.bounds.inset(1);

            if(isSelected && d.state->isActive) {
                Gfx::StrokeRect(fb, data.bounds, kSelectedBorder);
                Gfx::FillRect(fb, contentBounds, kSelectedFill);
            } else {
                Gfx::StrokeRect(fb, data.bounds, kUnselectedBorder);
                Gfx::FillRect(fb, contentBounds, kUnselectedFill);
            }

            // current value
            if(d.state->valueDirty) {
                UpdateValueString(data);
                d.state->valueDirty = false;
            }

            d.font->draw(d.state->displayBuf, fb, contentBounds, d.fontMode);
        }

        /**
         * @brief Handle selection event
         *
         * Cycle through step sizes, and if at the end, relinquish selection. Otherwise, just
         * relinquish selection.
         *
         * @return Whether we want to keep selection
         */
        static bool HandleSelection(const ComponentData &data) {
            auto state = data.numSpinner.state;

            // not active: select first step size and activate focus
            if(!state->isActive) {
                state->isActive = true;
            }
            // active: cycle through steps, or deactivate
            else {
                state->isActive = false;
            }

            return state->isActive;
        }

        /**
         * @brief Handle encoder events
         *
         * Scale the delta by the current step size, and apply it to the current value.
         */
        static void HandleEncoder(const ComponentData &data, const int delta, bool &needsDraw) {
            auto state = data.numSpinner.state;

            // bail if we're not selected
            if(!state->isActive) {
                return;
            }

            // determine step size (multiplier)
            auto multiplier = 1;

            // update value
            auto newValue = state->value + (multiplier * delta);

            if(state->isLimited) {
                if(newValue > state->maximum) {
                    newValue = state->maximum;
                }
                else if(newValue < state->minimum) {
                    newValue = state->minimum;
                }
            }

            const bool valueChanged = (state->value != newValue);

            if(valueChanged) {
                state->value = newValue;

                // force redraw
                state->valueDirty = true;
                needsDraw = true;

                // invoke callback
                if(data.numSpinner.valueChanged) {
                    data.numSpinner.valueChanged(newValue, data.numSpinner.context);
                }
            }
        }

    private:
        /**
         * @brief Update value string representation
         *
         * Formats the current value for display as a string.
         */
        static void UpdateValueString(const ComponentData &data) {
            auto state = data.numSpinner.state;

            // apply divisor

            // build up the format string 

            snprintf(state->displayBuf, sizeof(state->displayBuf), "%d mA", state->value);
        }

    private:
        /// Selected border color
        constexpr static const uint32_t kSelectedBorder{0xf};
        /// Selected fill color
        constexpr static const uint32_t kSelectedFill{0x1};
        /// Unselected border color
        constexpr static const uint32_t kUnselectedBorder{0x2};
        /// Unselected fill color
        constexpr static const uint32_t kUnselectedFill{0x0};
};
}

#endif
