#include "ScreenManager.h"
#include "Components/Base.h"
#include "Screen.h"

#include "App/Pinball/Task.h"

#include "Gfx/Types.h"
#include "Gfx/Framebuffer.h"
#include "Log/Logger.h"

using namespace Gui;

ScreenManager *ScreenManager::gShared{nullptr};

/**
 * @brief Initialize shared instance of screen manager
 */
void ScreenManager::Init() {
    static uint8_t gScreenManagerBuf[sizeof(ScreenManager)] __attribute__((aligned(alignof(ScreenManager))));
    auto ptr = reinterpret_cast<ScreenManager *>(gScreenManagerBuf);

    gShared = new (ptr) ScreenManager();
}

/**
 * @brief Initialize the screen manager.
 *
 * This sets up the animation frame timer. This timer fires periodically (at a fixed interval) and
 * will force re-rendering of the display.
 */
ScreenManager::ScreenManager() {
    // set up the timer
    static StaticTimer_t gAnimationTimer;
    this->animationTimer = xTimerCreateStatic("GUI animation timer",
        // one-shot timer mode (we'll reload it as needed)
        pdMS_TO_TICKS(kAnimationPeriod), pdFALSE,
        this, [](auto timer) {
            auto sm = reinterpret_cast<ScreenManager *>(pvTimerGetTimerID(timer));
            sm->advanceAnimationFrame();
        }, &gAnimationTimer);
    REQUIRE(this->animationTimer, "gui: %s", "failed to allocate timer");
}

/**
 * @brief Draws the UI.
 *
 * If an animation is in progress, we'll render it from a back buffer into the current display
 * buffer. Otherwise, the screen on top of the nav stack is simply rendered into the display main
 * framebuffer.
 */
void ScreenManager::draw() {
    // draw the screen
    if(!this->navStack.empty()) {
        auto screen = this->navStack.top();
        this->drawScreen(Gfx::Framebuffer::gMainBuffer, screen);
    }
}

/**
 * @brief Draw a screen
 *
 * Draw a screen into a framebuffer.
 *
 * @param fb Framebuffer to draw into
 * @param screen Screen to draw
 */
void ScreenManager::drawScreen(Gfx::Framebuffer &fb, Screen *screen) {
    // XXX: does the screen want the framebuffer clared?

    // draw each component in sequence
    for(auto component : screen->components) {
        component->draw(fb);
    }
}



/**
 * @brief Present a screen, replacing the navigation stack
 *
 * The existing navigation stack is emptied, then this screen is pushed on the navigation stack
 *
 * @param screen Screen to display
 * @param animation Which animation to use for this display
 */
void ScreenManager::present(Screen *screen, const PresentationAnimation animation) {
    // clear navigation stack
    while(!this->navStack.empty()) {
        //auto s = this->navStack.top();
        this->navStack.pop();
    }

    // insert at the top
    //screen->willPresent(screen);
    this->navStack.push(screen);

    // TODO: set up animation
    // force redraw
    this->requestDraw();
}

/**
 * @brief Update the state of animations.
 *
 * This is invoked whenever the animation timer expires. We'll update the animation time counter,
 * and request a redraw.
 */
void ScreenManager::advanceAnimationFrame() {
    // TODO: update animation state

    this->requestDraw();
}

/**
 * @brief Request the UI task redraws the GUI
 */
void ScreenManager::requestDraw() {
    // TODO: find a way to make this more generic
    App::Pinball::Task::NotifyTask(App::Pinball::Task::TaskNotifyBits::RedrawUI);
}
