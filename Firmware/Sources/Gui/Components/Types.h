#ifndef GUI_COMPONENTS_TYPES_H
#define GUI_COMPONENTS_TYPES_H

#include <bitflags.h>

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Namespace containing component implementations
 */
namespace Gui::Components {
/**
 * @brief Control drawing flags
 *
 * A combination of these bit fields may be specified to define how the controls should be drawn
 * on screen.
 */
enum class DrawFlags: uintptr_t {
    /// No special drawing
    None                                = 0,
    /// Control is selected
    Selected                            = (1 << 0),
};
ENUM_FLAGS_EX(DrawFlags, uintptr_t);
}

#endif
