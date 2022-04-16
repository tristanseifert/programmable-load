/**
 * @file
 *
 * @brief Version information screens
 *
 * This contains the screens for the instrument startup version, license information, and detailed
 * software and hardware information.
 */
#include "Screens.h"
#include "../Task.h"

#include "Gfx/Font.h"
#include "Gui/Components/StaticLabel.h"
#include "Util/HwInfo.h"

#include <BuildInfo.h>
#include <printf/printf.h>

using namespace App::Pinball;

/**
 * @brief Version splash screen
 *
 * Shown on startup, shows hardware and software version info
 */
const Gui::Screen *Screens::GetVersionSplash() {
    // fill in software version string
    static char gSwString[50];
    snprintf(gSwString, sizeof(gSwString), "%s/%s (%s)",
            gBuildInfo.gitBranch, gBuildInfo.gitHash, gBuildInfo.buildType);

    // fill in hardware version string
    static char gHwString[50];
    snprintf(gHwString, sizeof(gHwString), "Rev %u • S/N %s",
            Util::HwInfo::GetRevision(), Util::HwInfo::GetSerial());

    // define screen
    static const Gui::ComponentData gComponents[]{
        // heading
        {
            .type = Gui::ComponentType::StaticLabel,
            .bounds = {Gfx::MakePoint(0, 0), Gfx::MakeSize(255, 20)},
            .staticLabel = {
                .string = "Programmable Load",
                .font = &Gfx::Font::gGeneral_16_Bold,
                .fontMode = Gfx::FontRenderFlags::HAlignCenter,
            },
        },
        // hardware version
        {
            .type = Gui::ComponentType::StaticLabel,
            .bounds = {Gfx::MakePoint(0, 40), Gfx::MakeSize(58, 11)},
            .staticLabel = {
                .string = "Hardware:",
                .font = &Gfx::Font::gSmall,
                .fontMode = Gfx::FontRenderFlags::HAlignRight,
            },
        },
        {
            .type = Gui::ComponentType::StaticLabel,
            .bounds = {Gfx::MakePoint(60, 40), Gfx::MakeSize(190, 11)},
            .staticLabel = {
                .string = gHwString,
                .font = &Gfx::Font::gSmall,
                .fontMode = Gfx::FontRenderFlags::HAlignLeft,
            },
        },
        // software version
        {
            .type = Gui::ComponentType::StaticLabel,
            .bounds = {Gfx::MakePoint(0, 52), Gfx::MakeSize(58, 11)},
            .staticLabel = {
                .string = "Software:",
                .font = &Gfx::Font::gSmall,
                .fontMode = Gfx::FontRenderFlags::HAlignRight,
            },
        },
        {
            .type = Gui::ComponentType::StaticLabel,
            .bounds = {Gfx::MakePoint(60, 52), Gfx::MakeSize(190, 11)},
            .staticLabel = {
                .string = gSwString,
                .font = &Gfx::Font::gSmall,
                .fontMode = Gfx::FontRenderFlags::HAlignLeft,
            },
        },
    };
    static const Gui::Screen gScreen{
        .title = "Version Splash",
        .numComponents = 5,
        .components = gComponents,
        // pressing menu will open the home screen
        .menuPressed = [](auto screen, auto ctx){
            Task::NotifyTask(Task::TaskNotifyBits::ShowHomeScreen);
        },
    };

    return &gScreen;
}

const Gui::Screen *Screens::GetVersionSoftware() {
    static const Gui::ComponentData gComponents[]{
        {
            .type = Gui::ComponentType::StaticLabel,
            .bounds = {Gfx::MakePoint(0, 0), Gfx::MakeSize(255, 20)},
            .staticLabel = {
                .string = "Weed Smoker's Club",
                .font = &Gfx::Font::gGeneral_16_Bold,
                .fontMode = Gfx::FontRenderFlags::HAlignCenter,
            },
        },
    };
    static const Gui::Screen gScreen{
        .title = "Software Info",
        .numComponents = 1,
        .components = gComponents,
    };

    return &gScreen;
}

    return &gScreen;
}

