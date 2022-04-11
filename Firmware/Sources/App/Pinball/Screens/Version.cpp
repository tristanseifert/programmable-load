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
Gui::Screen *Screens::GetVersionSplash() {
    static Gui::Screen *screen{nullptr};
    if(!screen) {
        // static labels
        static Gui::Components::StaticLabel gHelloLabel(
            {Gfx::MakePoint(0, 0), Gfx::MakeSize(255, 20)},
            "Programmable Load",
            Gfx::Font::gGeneral_16_Bold, Gfx::Font::RenderFlags::HAlignCenter
        );
        static Gui::Components::StaticLabel gSwVersionLabel(
            {Gfx::MakePoint(0, 52), Gfx::MakeSize(58, 11)},
            "Software:", Gfx::Font::gFont_Small, Gfx::Font::RenderFlags::HAlignRight
        );
        static Gui::Components::StaticLabel gHwLabel(
            {Gfx::MakePoint(0, 40), Gfx::MakeSize(58, 11)},
            "Hardware:", Gfx::Font::gFont_Small, Gfx::Font::RenderFlags::HAlignRight
        );

        // software version
        static char gVersionString[50];
        snprintf(gVersionString, sizeof(gVersionString), "%s/%s (%s)",
                gBuildInfo.gitBranch, gBuildInfo.gitHash, gBuildInfo.buildType);

        static Gui::Components::StaticLabel gSwVersion(
            {Gfx::MakePoint(60, 52), Gfx::MakeSize(190, 11)},
            gVersionString,
            Gfx::Font::gFont_Small
        );

        // hardware version and info
        static char gHwString[50];
        snprintf(gHwString, sizeof(gHwString), "Rev %u • S/N %s",
                Util::HwInfo::GetRevision(), Util::HwInfo::GetSerial());

        static Gui::Components::StaticLabel gHw(
            {Gfx::MakePoint(60, 40), Gfx::MakeSize(190, 11)},
            gHwString,
            Gfx::Font::gFont_Small
        );

        // declare screen
        static etl::array<Gui::Components::Base *, 5> gComponents{{
            &gHelloLabel, &gSwVersionLabel, &gSwVersion, &gHwLabel, &gHw
        }};
        static Gui::Screen gVersionScreen{
            .title = "Version Info",
            .components = gComponents,
            // pressing menu will open the home screen
            .menuPressed = [](auto screen, auto ctx){
                Task::NotifyTask(Task::TaskNotifyBits::ShowHomeScreen);
            },
        };

        screen = &gVersionScreen;
    }

    return screen;
}

