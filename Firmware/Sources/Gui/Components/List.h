#ifndef GUI_COMPONENTS_LIST_H
#define GUI_COMPONENTS_LIST_H

#include "Scrollbar.h"
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
            contentBounds.size.width -= Scrollbar::kBarWidth;

            auto scrollbarBounds = contentBounds;
            scrollbarBounds.size.width = Scrollbar::kBarWidth;
            scrollbarBounds.origin.x += contentBounds.size.width;

            // draw the scrollbar
            Scrollbar::Draw(fb, scrollbarBounds, d.state->selectedRow, numRows);

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

            if((startRow + rowsPerScreen) >= numRows) {
                if(startRow) {
                    startRow -= 1;
                }
            }

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

        /**
         * @brief Handle encoder event
         *
         * This scrolls the list according to the specified delta, limiting it to the start (0) or
         * end (count) items.
         *
         * @param delta Relative change of encoder value
         */
        static void HandleEncoder(const ComponentData &data, const int delta, bool &needsDraw) {
            if(!delta) {
                return;
            }

            auto &d = data.list;
            const auto numRows = d.getNumRows(d.context);

            auto newIndex = static_cast<long>(d.state->selectedRow) + delta;
            if(newIndex < 0) {
                newIndex = 0;
            }
            if(newIndex >= numRows) {
                newIndex = numRows - 1;
            }

            d.state->selectedRow = static_cast<size_t>(newIndex);
            needsDraw = true;
        }
};
}

#endif
