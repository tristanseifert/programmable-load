/**
 * @file
 *
 * @brief Main screen
 *
 * Helper to render the main screen of the device.
 */
#include "Screens.h"
#include "../Task.h"

#include "App/Control/Task.h"
#include "Usb/Task.h"

#include "Gfx/Font.h"
#include "Gfx/Icon.h"
#include "Gui/ScreenManager.h"
#include "Gui/Components/List.h"
#include "Rtos/Rtos.h"

#include <printf/printf.h>

using namespace App::Pinball;

static const Gui::Screen *GetMenuScreen();

/// String buffer for voltage label
static char gVoltageBuffer[16];
/// String buffer for current label
static char gCurrentBuffer[16];

/**
 * @brief Main screen components
 *
 * This defines the components on the main screen of the instrument.
 */
static Gui::ComponentData gMainComponents[]{
    // input voltage
    {
        .type = Gui::ComponentType::StaticLabel,
        .bounds = {Gfx::MakePoint(20, 4), Gfx::MakeSize(120, 31)},
        .staticLabel = {
            .string = gVoltageBuffer,
            .font = &Gfx::Font::gNumbers_XL,
            .fontMode = Gfx::FontRenderFlags::HAlignRight,
        },
    },
    // input current
    {
        .type = Gui::ComponentType::StaticLabel,
        .bounds = {Gfx::MakePoint(20, 34), Gfx::MakeSize(120, 31)},
        .staticLabel = {
            .string = gCurrentBuffer,
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
    // divider for badge/mode area on left
    {
        .type = Gui::ComponentType::Divider,
        .bounds = {Gfx::MakePoint(18, 0), Gfx::MakeSize(1, 64)},
        .divider = {
            .color = 0x2,
        },
    },
    // sample indicator (toggles for each alternating sampling time)
    {
        .type = Gui::ComponentType::StaticLabel,
        .bounds = {Gfx::MakePoint(188, 40), Gfx::MakeSize(24, 24)},
        .staticLabel = {
            .string = "※",
            .font = &Gfx::Font::gNumbers_L,
            .fontMode = Gfx::FontRenderFlags::HAlignLeft,
        },
    },
    // USB connectivity icon
    {
        .type = Gui::ComponentType::StaticIcon,
        .bounds = {Gfx::MakePoint(0, 48), Gfx::MakeSize(16, 16)},
        .staticIcon = {
            .icon = &Gfx::Icon::gMainBadgeUsb,
            .hideIcon = true,
        },
    },
    // external sense
    {
        .type = Gui::ComponentType::StaticIcon,
        .bounds = {Gfx::MakePoint(0, 32), Gfx::MakeSize(16, 16)},
        .staticIcon = {
            .icon = &Gfx::Icon::gMainBadgeVExt,
            .hideIcon = true,
        },
    },
};

/**
 * @brief Screen update timer
 *
 * This timer fires periodically in order to force the screen to get redrawn, and thus the display
 * updated with the current voltage/current/temperature readings.
 */
static TimerHandle_t gUpdateTimer{nullptr};
/// Interval for update timer (milliseconds)
static const constexpr size_t kUpdateTimerInterval{74};
/// Sampling flag, toggled every time the timer is invoked
static bool gSamplingFlag{false};

/**
 * @brief Update the contents of the main screen
 *
 * Format the voltage and current values for display.
 */
static void UpdateMainScreen(const Gui::Screen *screen) {
    // format voltage
    const auto volts = App::Control::Task::GetInputVoltage();
    snprintf(gVoltageBuffer, sizeof(gVoltageBuffer), "%d.%02d V", volts / 1000,
            (volts % 1000) / 10);

    // format current
    const auto current = App::Control::Task::GetInputCurrent();

    if(current < 1'000'000) { // format as mA below 1A
        snprintf(gCurrentBuffer, sizeof(gCurrentBuffer), "%d.%02d mA", current / 1000,
                (current % 1000) / 10);
    } else {
        const auto ma = current / 1000;
        snprintf(gCurrentBuffer, sizeof(gCurrentBuffer), "%d.%03d A", ma / 1000,
                (ma % 1000));
    }

    // toggle the sample indicator every time we update
    auto &sampleIndicator = gMainComponents[4];
    sampleIndicator.staticLabel.string = gSamplingFlag ? "※" : " ";

    // icons (on the left side)
    auto &usbIcon = gMainComponents[5], &extSenseIcon = gMainComponents[6];

    usbIcon.staticIcon.hideIcon = !UsbStack::Task::GetIsConnected();
    extSenseIcon.staticIcon.hideIcon = !App::Control::Task::GetIsExternalSenseActive();
}

/**
 * @brief Get the unit main screen
 */
const Gui::Screen *Screens::GetMainScreen() {
    // perform one-time initialization
    if(!gUpdateTimer) {
        static StaticTimer_t gTimerBuf;

        gUpdateTimer = xTimerCreateStatic("Main screen update timer",
            pdMS_TO_TICKS(kUpdateTimerInterval), pdTRUE,
            nullptr, [](auto timer) {
                gSamplingFlag = !gSamplingFlag;
                App::Pinball::Task::NotifyTask(App::Pinball::Task::TaskNotifyBits::RedrawUI);
            }, &gTimerBuf);
        REQUIRE(gUpdateTimer, "failed to allocate timer");

    }

    // return the screen structure
    static const Gui::Screen gScreen{
        .title = "Main",
        .numComponents = (sizeof(gMainComponents) / sizeof(gMainComponents[0])),
        .components = gMainComponents,
        // when we're about to appear, start the update timer
        .willPresent = [](auto screen, auto ctx) {
            xTimerReset(gUpdateTimer, portMAX_DELAY);
        },
        // when about to disappear, stop the update timer
        .willDisappear = [](auto screen, auto ctx) {
            xTimerStop(gUpdateTimer, portMAX_DELAY);
        },
        // open instrument menu when menu is pressed
        .menuPressed = [](auto screen, auto ctx){
            auto menu = GetMenuScreen();
            Gui::ScreenManager::Push(menu, Gui::ScreenManager::Animation::SlideUp);
        },
        // update the current display state
        .willDraw = [](auto screen, auto ctx) {
            UpdateMainScreen(screen);
        },
    };

    return &gScreen;
}



constexpr static const size_t kMenuRows{4};

/**
 * @brief Draw a row in the main menu list
 *
 * This handles drawing rows: these fall into two general categories, ones with an accessory (such
 * as the current value of a property) and those without.
 */
static void DrawMenuRow(Gfx::Framebuffer &fb, const Gfx::Rect bounds, const size_t rowIndex,
        const bool isSelected, void *context) {
    using FontRenderFlags = Gfx::FontRenderFlags;

    // calculate styles (taking into account selection)
    FontRenderFlags baseFlags = isSelected ? FontRenderFlags::Invert : FontRenderFlags::None;

    /*
     * Primary text labels for each row and whether that row has a secondary item or not
     */
    static const etl::array<const char *, kMenuRows> kTitles{{
        "Voltage Sense", "Mode",
        "System Setup",
    }};
    static const etl::array<const bool, kMenuRows> kHasAccessory{{
        true, true, false, false
    }};

    // draw the primary label
    auto titleBounds = bounds;
    titleBounds.origin.x += 2;
    titleBounds.size.width -= 4;

    Gfx::Font::gGeneral_16_Bold.draw(kTitles[rowIndex], fb, titleBounds,
            FontRenderFlags::HAlignLeft | baseFlags);

    // draw accessory
    if(kHasAccessory[rowIndex]) {
        switch(rowIndex) {
            // Voltage sense mode
            case 0: {
                const auto text = App::Control::Task::GetIsExternalSenseActive() ? "External" :
                    "Internal";
                Gfx::Font::gGeneral_16_Condensed.draw(text, fb, titleBounds,
                        FontRenderFlags::HAlignRight | baseFlags);
                break;
            }

            // shouldn't happen
            default:
                break;
        }
    }
}

/**
 * @brief Handle list view selection
 */
static void HandleMenuRowSelection(const size_t index, void *context) {
    // voltage sense
    if(index == 0) {
        auto useExternal = !App::Control::Task::GetIsExternalSenseActive();
        App::Control::Task::SetExternalSenseActive(useExternal);
    }

    // XXX: find a better way than doing this manually
    App::Pinball::Task::NotifyTask(App::Pinball::Task::TaskNotifyBits::RedrawUI);
}

/**
 * @brief Get the main menu screen
 *
 * This consists of a full-screen list that allows configuring the operation of the system, and
 * serves as the "portal" to various other system settings menus.
 */
static const Gui::Screen *GetMenuScreen() {
    // stuff for list
    static Gui::Components::ListState gListState;

    // define the screen
    static const Gui::ComponentData gComponents[]{
        // top divider
        {
            .type = Gui::ComponentType::Divider,
            .bounds = {Gfx::MakePoint(0, 0), Gfx::MakeSize(256, 1)},
            .divider = {
                .color = 0x4,
            },
        },
        // table/list view (for options)
        {
            .type = Gui::ComponentType::List,
            .bounds = {Gfx::MakePoint(0, 1), Gfx::MakeSize(256, 63)},
            .list = {
                .state = &gListState,
                .rowHeight = 21,
                .getNumRows = [](auto ctx) -> size_t {
                    return kMenuRows;
                },
                .drawRow = DrawMenuRow,
                .rowSelected = HandleMenuRowSelection,
            },
        },
    };

    // return the screen structure
    static const Gui::Screen gScreen{
        .title = "Main Menu",
        .numComponents = (sizeof(gComponents) / sizeof(gComponents[0])),
        .components = gComponents,
        // use the "slide down" animation
        .menuPressed = [](auto screen, auto ctx){
            Gui::ScreenManager::Pop(Gui::ScreenManager::Animation::SlideDown);
        },
    };

    return &gScreen;
}
