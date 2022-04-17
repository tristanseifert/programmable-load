#ifndef GUI_COMPONENTS_LIST_H
#define GUI_COMPONENTS_LIST_H

#include "../Screen.h"
#include "Log/Logger.h"
#include "Gfx/Primitives.h"

namespace Gui::Components {
/**
 * @brief List view state
 *
 * Contains the state of a list view; these are allocated separately for each list and are then
 * referenced by its definition.
 */
struct ListState {
    /// Currently selected row
    size_t selectedRow{0};
};


/**
 * @brief Scrollable list view
 *
 * Handles displaying a list of items (each which has a fixed width) which can be scrollable.
 *
 * The current state of the list is stored in the "State" struct, which must be separately 
 * allocated for each list.
 */
class List {
    public:
        /**
         * @brief Draw the list view
         */
        static void Draw(Gfx::Framebuffer &fb, const ComponentData &data) {
            // validate data
            auto &d = data.list;
            REQUIRE(d.state, "missing list state");

            // query number of rows and calculate bounds
            const auto numRows = d.getNumRows(d.context);

            auto contentBounds = data.bounds;
            contentBounds.size.width -= 10; // TODO: scrollbar width not hardcoded

            auto scrollbarBounds = contentBounds;
            scrollbarBounds.size.width = 10;
            scrollbarBounds.origin.x += contentBounds.size.width;

            auto scrollTrackBounds = scrollbarBounds;
            scrollTrackBounds.size.width -= 1;
            scrollTrackBounds.origin.x += 1;

            // draw the scrollbar
            Gfx::DrawLine(fb, scrollbarBounds.origin, Gfx::MakePoint<int>(scrollbarBounds.origin.x,
                        scrollbarBounds.origin.y + scrollbarBounds.size.height), 0x9);

            Gfx::FillRect(fb, scrollTrackBounds, 0x1);

            if(numRows) {
                // calculate the knob height
                auto knobHeight = scrollbarBounds.size.height / numRows;
                if(knobHeight < 4) {
                    knobHeight = 4;
                }

                // calculate knob's position and draw
                const auto knobRange = scrollTrackBounds.size.height - knobHeight;
                const uint16_t yOffset = static_cast<float>(knobRange) *
                    (static_cast<float>(d.state->selectedRow) / static_cast<float>(numRows));

                const Gfx::Rect knobBounds{
                    Gfx::MakePoint<int>(scrollTrackBounds.origin.x,
                            scrollTrackBounds.origin.y + yOffset),
                    Gfx::MakeSize<int>(scrollTrackBounds.size.width, knobHeight),
                };

                Gfx::FillRect(fb, knobBounds, 0xD);
            }

            /*
             * Figure out which rows to draw. This is very basic and tries to center the currently
             * selected row, if possible.
             */
            size_t rowsPerScreen = (data.bounds.size.height + d.rowHeight - 1) / d.rowHeight;

            size_t startRow = d.state->selectedRow;
            if(startRow) {
                startRow -= 1;
            }

            size_t maxRow = (startRow + rowsPerScreen) < numRows ? (startRow + rowsPerScreen) :
                numRows;

            // calculate bounds for the rows
            auto rowBounds = contentBounds;
            auto remainingHeight = rowBounds.size.height;
            rowBounds.size.height = d.rowHeight;

            for(size_t i = startRow; i < maxRow; i++) {
                const auto isSelected = (d.state->selectedRow == i);

                // clear, then draw the row
                Gfx::FillRect(fb, rowBounds, isSelected ? 0xf : 0x0);
                d.drawRow(fb, rowBounds, i, isSelected, d.context);

                // advance bounds down
                remainingHeight -= rowBounds.size.height;
                rowBounds.origin.y += d.rowHeight;
                rowBounds.size.height = (remainingHeight > d.rowHeight) ?
                    d.rowHeight : remainingHeight;
            }
        }

        /**
         * @brief Handle selection event
         *
         * Process a selection event while the list is key. We'll forward this to the handler for
         * row selections.
         */
        static void HandleSelection(const ComponentData &data) {
            auto &d = data.list;
            d.rowSelected(d.state->selectedRow, d.context);
        }
};
}

#endif
