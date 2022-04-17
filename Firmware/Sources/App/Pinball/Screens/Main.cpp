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

#include "Gfx/Font.h"
#include "Gui/Components/StaticLabel.h"
#include "Rtos/Rtos.h"

#include <printf/printf.h>

using namespace App::Pinball;

/// String buffer for voltage label
static char gVoltageBuffer[16];
/// String buffer for current label
static char gCurrentBuffer[16];

/**
 * @brief Screen update timer
 *
 * This timer fires periodically in order to force the screen to get redrawn, and thus the display
 * updated with the current voltage/current/temperature readings.
 */
static TimerHandle_t gUpdateTimer{nullptr};
/// Interval for update timer (milliseconds)
static const constexpr size_t kUpdateTimerInterval{100};

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
}

/**
 * @brief Get the unit main screen
 *
 * @TODO get data from actual sources; this is just a mockup right now
 */
const Gui::Screen *Screens::GetMainScreen() {
    // perform one-time initialization
    if(!gUpdateTimer) {
        static StaticTimer_t gTimerBuf;

        gUpdateTimer = xTimerCreateStatic("Main screen update timer",
            pdMS_TO_TICKS(kUpdateTimerInterval), pdTRUE,
            nullptr, [](auto timer) {
                App::Pinball::Task::NotifyTask(App::Pinball::Task::TaskNotifyBits::RedrawUI);
            }, &gTimerBuf);
        REQUIRE(gUpdateTimer, "failed to allocate timer");

    }

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
    };
    static const Gui::Screen gScreen{
        .title = "Main",
        .numComponents = 4,
        .components = gComponents,
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
            Logger::Notice("XXX: Open instrument menu");
        },
        // update the current display state
        .willDraw = [](auto screen, auto ctx) {
            UpdateMainScreen(screen);
        },
    };

    return &gScreen;
}
