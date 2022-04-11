#include "ScreenManager.h"
#include "Components/Base.h"
#include "EasingFunctions.h"
#include "Screen.h"

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
        // TODO: check if screen wants to be redrawn
        this->drawScreen(gAnimationBuffer, screen);
        this->drawAnimationFrame();
    }
    // otherwise, draw the screen to the front buffer
    else {
        // clear backbuffer if we just finished an animation
        if(!this->isAnimating && this->animationComplete) {
            Gfx::Framebuffer::gMainBuffer.clear();
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
    // clear the back buffer
    gAnimationBuffer.clear();

    // reset animation state
    this->isAnimating = true;
    this->animationComplete = false;
    this->animationProgress = 0.f;
    this->currentAnimation = animation;

    switch(animation) {
        case Animation::SlideIn:
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
 */
void ScreenManager::drawAnimationFrame() {
    float progress;

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
            origin.x = (-256.f * progress);
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
void ScreenManager::drawScreen(Gfx::Framebuffer &fb, Screen *screen) {
    // XXX: does the screen want the framebuffer clared first?

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
void ScreenManager::present(Screen *screen, const Animation animation) {
    // notify the topmost controller it will disappear
    if(!this->navStack.empty()) {
        auto top = this->navStack.top();

        if(top->willDisappear) {
            top->willDisappear(top, top->callbackContext);
        }
        // XXX: call didDisappear?

        // clear navigation stack
        this->navStack.clear();
    }

    // make the new controller the first controller visible
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
    }

    // force redraw
    this->requestDraw();
}

/**
 * @brief Request the UI task redraws the GUI
 */
void ScreenManager::requestDraw() {
    // TODO: find a way to make this more generic
    App::Pinball::Task::NotifyTask(App::Pinball::Task::TaskNotifyBits::RedrawUI);
}
