#ifndef GUI_SCREENMANAGER_H
#define GUI_SCREENMANAGER_H

#include <stddef.h>
#include <stdint.h>

#include "Screen.h"
#include "Rtos/Rtos.h"

#include <etl/stack.h>

namespace Gfx {
class Framebuffer;
}

namespace Gui {
/**
 * @brief Handles displaying of GUI screens
 *
 * The screen manager handles the drawing, updating, and distributing input (which includes the
 * selection handling) for a screen. Additionally, it provides facilities to present screens,
 * optionally with animations.
 */
class ScreenManager {
    public:
        /**
         * @brief Animations used for presenting a screen
         */
        enum class Animation {
            /// No animation
            None                        = 0,
            /// Slide up from bottom
            SlideUp                     = 1,
            /// Slide down from the top
            SlideDown                   = 2,
            /// Slide in from right
            SlideIn                     = 3,
            /// Slide out to the left
            SlideOut                    = 4,
        };

    public:
        static void Init();

        static void Draw() {
            gShared->draw();
        }

        /**
         * @brief Present the specified screen without animation
         *
         * The navigation stack is reset, and this screen is placed at the bottom.
         *
         * @param screen Screen to display
         */
        inline static void Present(Screen *screen) {
            Present(screen, Animation::None);
        }
        /**
         * @brief Present the specified screen
         *
         * Replace the contents of the navigation stack with the specified screen. If an animation
         * is specified, the end result is the same as if the screen were pushed with the specified
         * animation.
         */
        inline static void Present(Screen *screen, const Animation animation) {
            gShared->present(screen, animation);
        }

        /**
         * @brief Show the nav stack menu
         *
         * This will populate and display the navigation stack menu, which allows the user to
         * select a view in the nav stack that we'll "pop" back to.
         */
        inline static void OpenNavStackMenu() {
            gShared->openNavMenu();
        }

    private:
        ScreenManager();

        void draw();
        void drawScreen(Gfx::Framebuffer &fb, Screen *screen);

        void prepareAnimation(const Animation);
        void drawAnimationFrame();
        void advanceAnimationFrame();

        void push(Screen *screen, const Animation animation);
        void present(Screen *screen, const Animation animation);

        void openNavMenu();

        void requestDraw();

    private:
        /// Global screen manager instance
        static ScreenManager *gShared;

        /// Maximum depth of navigation stack
        constexpr static const size_t kNavStackDepth{8};
        /// Navigation stack
        etl::stack<Screen *, kNavStackDepth> navStack;

        /// Animation period (in msec)
        constexpr static const size_t kAnimationPeriod{30};
        /// Timer used to drive animations
        TimerHandle_t animationTimer;
        /**
         * @brief Progress of the current animation
         *
         * A percentage value, between [0, 1] that specifies the progress of the currently
         * proceeding animation.
         */
        float animationProgress;
        /// Step (increment) for the animation progress for this animation
        float animationProgressStep;
        /// is an animation in progress?
        bool isAnimating{false};
        /// did an animation just complete?
        bool animationComplete{false};
        /// current animation
        Animation currentAnimation{Animation::None};

        /**
         * @brief Secondary (back) buffer
         *
         * This is used for animations, as a second "layer" to hold the new content, which is then
         * blitted over the old content, gradually replacing it.
         */
        static Gfx::Framebuffer gAnimationBuffer;
};
}

#endif
