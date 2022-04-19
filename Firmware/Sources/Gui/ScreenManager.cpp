#include "ScreenManager.h"
#include "Components/Base.h"
#include "EasingFunctions.h"
#include "InputManager.h"
#include "Screen.h"

#include "App/Pinball/Beeper.h"
#include "App/Pinball/Task.h"

#include "Gfx/Types.h"
#include "Gfx/Framebuffer.h"
#include "Log/Logger.h"

#include <etl/array.h>
#include <math.h>

using namespace Gui;

ScreenManager *ScreenManager::gShared{nullptr};

// XXX: do not hard code these values!
static etl::array<uint8_t, (256*64)/2> gAnimationBufferData;
Gfx::Framebuffer ScreenManager::gAnimationBuffer{
    .size = {
        .width = 256,
        .height = 64,
    },
    .data = gAnimationBufferData,
    .stride = (256/2),
};

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
    // get the screen to render (the topmost/active one)
    if(this->navStack.empty()) {
        return;
    }

    auto screen = this->navStack.top();

    // if an animation is in progress, render the controller to the back buffer and blit it
    if(this->isAnimating) {
        this->drawAnimationFrame(screen);
    }
    // otherwise, draw the screen to the front buffer
    else {
        // clear display buffer if we just finished an animation
        if(this->needsBufferClear) {
            Gfx::Framebuffer::gMainBuffer.clear();
            this->needsBufferClear = false;
        }

        // TODO: check if screen wants to be redrawn
        this->drawScreen(Gfx::Framebuffer::gMainBuffer, screen);

        // if we just finished an animation, invoke the screen's "did appear" callback
        if(!this->isAnimating && this->animationComplete) {
            if(screen->didPresent) {
                screen->didPresent(screen, screen->callbackContext);
            }

            this->animationComplete = false;
            this->currentAnimation = Animation::None;
        }
    }
}

/**
 * @brief Initialize animation state
 *
 * The state of the animation is reset to default, any initial actions are done to the framebuffer
 * contents, and the animation timer is armed.
 */
void ScreenManager::prepareAnimation(const Animation animation) {
    // clear the back buffer (if it's an IN animation)
    if(animation == Animation::SlideIn || animation == Animation::SlideUp) {
        gAnimationBuffer.clear();
    }

    // reset animation state
    this->isAnimating = true;
    this->animationComplete = false;
    this->animationProgress = 0.f;
    this->currentAnimation = animation;

    switch(animation) {
        case Animation::SlideIn:
        case Animation::SlideOut:
            this->animationProgressStep = 0.025f;
            break;

        default:
            this->animationProgressStep = 0.05f;
            break;
    }

    // TODO: perform per animation first time drawing?

    // restart the animation timer
    auto err = xTimerReset(this->animationTimer, 0);
    REQUIRE(err == pdPASS, "gui: %s", "failed to re-arm timer");
}

/**
 * @brief Draw an animation frame
 *
 * This will blit the new screen (from the animation buffer) onto the display framebuffer, before
 * it will be transferred.
 *
 * It takes advantage of the fact that all our animations are just some sort of sliding.
 *
 * @param screen The screen that's currently the top of the nav stack
 */
void ScreenManager::drawAnimationFrame(const Screen *screen) {
    float progress;

    /*
     * Render the screen to the appropriate buffer.
     *
     * If the animation is an OUT (SlideOut or SlideDown) we'll render the topmost screen to the
     * main buffer, then blit the contents of the back buffer (which contain the old screen)
     * without redrawing that.
     *
     * Otherwise, we draw the new screen onto the back buffer, then blit that on top of the display
     * buffer (which is unchanged, and contains the previous top view.)
     */
    if(this->currentAnimation == Animation::SlideOut ||
            this->currentAnimation == Animation::SlideDown) {
        // TODO: selectively clear the buffer
        Gfx::Framebuffer::gMainBuffer.clear();
        this->drawScreen(Gfx::Framebuffer::gMainBuffer, screen);
    } else {
        // TODO: check if screen wants to be redrawn
        this->drawScreen(gAnimationBuffer, screen);
    }

    // apply an easing function to the current animation progress
    switch(this->currentAnimation) {
        case Animation::SlideDown: [[fallthrough]];
        case Animation::SlideUp:
            progress = EasingFunctions::InOutQuad(this->animationProgress);
            break;

        case Animation::SlideIn: [[fallthrough]];
        case Animation::SlideOut:
            progress = EasingFunctions::InOutQuart(this->animationProgress);
            break;

        default:
            progress = this->animationProgress;
            break;
    }

    // calculate the top left origin for the blit
    Gfx::Point origin{0, 0};

    switch(this->currentAnimation) {
        case Animation::SlideUp:
            origin.y = 64 - (64.f * progress);
            break;
        case Animation::SlideDown:
            origin.y = (64.f * progress);
            break;

        case Animation::SlideIn:
            origin.x = 256 - (256.f * progress);
            break;
        case Animation::SlideOut:
            origin.x = (256.f * progress);
            break;

        default:
            break;
    }

    // do the blit
    Gfx::Framebuffer::gMainBuffer.blit4Bpp(gAnimationBuffer, origin);
}

/**
 * @brief Update the state of animations.
 *
 * This is invoked whenever the animation timer expires. We'll update the animation time counter,
 * and request a redraw.
 */
void ScreenManager::advanceAnimationFrame() {
    // update animation state
    this->animationProgress += this->animationProgressStep;

    // terminate the animation or reset timer
    if(this->animationProgress >= 1.f) {
        this->isAnimating = false;
        this->animationComplete = true;
        this->needsBufferClear = true;
    } else {
        auto err = xTimerReset(this->animationTimer, 0);
        REQUIRE(err == pdPASS, "gui: %s", "failed to re-arm timer");
    }

    this->requestDraw();
}

/**
 * @brief Draw a screen
 *
 * Draw a screen into a framebuffer.
 *
 * @param fb Framebuffer to draw into
 * @param screen Screen to draw
 */
void ScreenManager::drawScreen(Gfx::Framebuffer &fb, const Screen *screen) {
    REQUIRE(screen, "gui: %s", "invalid screen");

    // XXX: does the screen want the framebuffer clared first?

    if(screen->willDraw) {
        screen->willDraw(screen, screen->callbackContext);
    }

    // draw each component in sequence
    for(size_t i = 0; i < screen->numComponents; i++) {
        const auto &cd = screen->components[i];
        if(cd.isHidden) continue;

        Components::Draw(fb, cd);
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
void ScreenManager::present(const Screen *screen, const Animation animation) {
    // remove all existing controllers
    if(!this->navStack.empty()) {
        auto top = this->navStack.top();

        if(top->willDisappear) {
            top->willDisappear(top, top->callbackContext);
        }
        // XXX: call didDisappear?
    }
    this->navStack.clear();

    // then push as normal
    this->push(screen, animation);
}

/**
 * @brief Push a screen to the top of the navigation hierarchy.
 *
 * @param screen Screen to display
 * @param animation Which animation to use for this display
 */
void ScreenManager::push(const Screen *screen, const Animation animation) {
    // notify the topmost controller it will disappear
    if(!this->navStack.empty()) {
        auto top = this->navStack.top();

        if(top->willDisappear) {
            top->willDisappear(top, top->callbackContext);
        }
        // XXX: call didDisappear?
    }

    // push the new screen
    if(screen->willPresent) {
        screen->willPresent(screen, screen->callbackContext);
    }
    this->navStack.push(screen);

    // prepare the animation
    if(animation != Animation::None) {
        this->prepareAnimation(animation);
    } else {
        if(screen->didPresent) {
            screen->didPresent(screen, screen->callbackContext);
        }

        this->needsBufferClear = true;
    }

    // force redraw and reset selection
    this->requestDraw();

    InputManager::ResetSelection(screen);

    // update indicators (for menu button light)
    App::Pinball::Task::NotifyTask(App::Pinball::Task::TaskNotifyBits::UpdateIndicators);
}

/**
 * @brief Pop a screen off the navigation stack
 *
 * Make the topmost (currently visible) screen disappear.
 */
void ScreenManager::pop(const Animation animation) {
    // ensure we have a screen to pop
    if(this->navStack.size() < 2) {
        return;
    }

    /*
     * If we're using an animation, render the screen we're about to disappear into our back buffer
     * so we can draw it on top of the new "top" of the navigation stack as it disappears.
     */
    const Screen *top = this->navStack.top();

    if(animation != Animation::None) {
        gAnimationBuffer.clear();
        this->drawScreen(gAnimationBuffer, top);
        // XXX: call didDisappear
    }

    if(top->willDisappear) {
        top->willDisappear(top, top->callbackContext);
    }

    // get the screen to show after
    this->navStack.pop();
    const Screen *revealed = this->navStack.top();

    if(revealed->willPresent) {
        revealed->willPresent(revealed, revealed->callbackContext);
    }
    InputManager::ResetSelection(revealed);

    // prepare the animation
    if(animation != Animation::None) {
        this->prepareAnimation(animation);
    } else {
        this->needsBufferClear = true;
    }

    this->requestDraw();

    // update indicators (for menu button light)
    App::Pinball::Task::NotifyTask(App::Pinball::Task::TaskNotifyBits::UpdateIndicators);
}

/**
 * @brief Process a screen's menu action
 *
 * If the screen has a menu action specified, we'll invoke that. Otherwise, pop a screen off the
 * navigation stack (with animation) if there is one available. If no screens are available to pop
 * (e.g. the current screen is the topmost) we do nothing.
 */
void ScreenManager::doMenuAction() {
    // ensure we have a screen
    if(this->navStack.empty()) {
        return;
    }
    auto screen = this->navStack.top();

    // invoke its menu action, if specified
    if(screen->menuPressed) {
        return screen->menuPressed(screen, screen->callbackContext);
    }
    // otherwise, pop this controller
    if(this->navStack.size() == 1) {
        App::Pinball::Beeper::Play(App::Pinball::Beeper::kInvalidButtonMelody);
        return;
    }

    this->pop(Animation::SlideOut);
}

/**
 * @brief Open the navigation stack menu
 *
 * This populates the list of all navigation controllers on screen, in the navigation screen menu
 * that will then be pushed to the display.
 */
void ScreenManager::openNavMenu() {
    // TODO: implement
    Logger::Warning("TODO: open GUI nav menu");
}

/**
 * @brief Request the UI task redraws the GUI
 */
void ScreenManager::requestDraw() {
    // TODO: find a way to make this more generic
    App::Pinball::Task::NotifyTask(App::Pinball::Task::TaskNotifyBits::RedrawUI);
}
