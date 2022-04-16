/**
 * @file
 *
 * @brief UI screen definitions
 *
 * This file contains definitions for many of the top level screens exposed in the UI.
 */
#ifndef APP_PINBALL_SCREENS_SCREENS_H
#define APP_PINBALL_SCREENS_SCREENS_H

#include "Gui/Screen.h"

namespace App::Pinball {
struct Screens {
    static const Gui::Screen *GetVersionSplash();
    static const Gui::Screen *GetVersionSoftware();

    static const Gui::Screen *GetMainScreen();
};
}

#endif
