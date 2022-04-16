/**
 * @file
 *
 * @brief Main screen
 *
 * Helper to render the main screen of the device.
 */
#include "Screens.h"
#include "../Task.h"

#include "Gfx/Font.h"
#include "Gui/Components/StaticLabel.h"

#include <printf/printf.h>

using namespace App::Pinball;


/**
 * @brief Get the unit main screen
 *
 * @TODO get data from actual sources; this is just a mockup right now
 */
const Gui::Screen *Screens::GetMainScreen() {
    // define screen
    static const Gui::ComponentData gComponents[]{
        // divider for mode
        {
            .type = Gui::ComponentType::Divider,
            .bounds = {Gfx::MakePoint(18, 0), Gfx::MakeSize(1, 64)},
            .divider = {
                .color = 0x2,
            },
        },
        // input voltage
        {
            .type = Gui::ComponentType::StaticLabel,
            .bounds = {Gfx::MakePoint(20, 4), Gfx::MakeSize(120, 31)},
            .staticLabel = {
                .string = "0.00 V",
                .font = &Gfx::Font::gNumbers_XL,
                .fontMode = Gfx::FontRenderFlags::HAlignRight,
            },
        },
        // input current
        {
            .type = Gui::ComponentType::StaticLabel,
            .bounds = {Gfx::MakePoint(20, 34), Gfx::MakeSize(120, 31)},
            .staticLabel = {
                .string = "0.00 mA",
                .font = &Gfx::Font::gNumbers_XL,
                .fontMode = Gfx::FontRenderFlags::HAlignRight,
            },
        },
        // temperature
        {
            .type = Gui::ComponentType::StaticLabel,
            .bounds = {Gfx::MakePoint(205, 40), Gfx::MakeSize(50, 24)},
            .staticLabel = {
                .string = "24 °C",
                .font = &Gfx::Font::gNumbers_L,
                .fontMode = Gfx::FontRenderFlags::HAlignRight,
            },
        },
    };
    static const Gui::Screen gScreen{
        .title = "Main",
        .numComponents = 4,
        .components = gComponents,
        // open instrument menu when menu is pressed
        .menuPressed = [](auto screen, auto ctx){
            Logger::Notice("XXX: Open instrument menu");
        },
    };

    return &gScreen;
}
