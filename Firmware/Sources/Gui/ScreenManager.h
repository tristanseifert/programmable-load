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
        enum class PresentationAnimation {
            /// No animation
            None                        = 0,
            /// Slide up from bottom
            SlideUp                     = 1,
            /// Slide in from right
            SlideIn                     = 2,
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
            Present(screen, PresentationAnimation::None);
        }
        /**
         * @brief Present the specified screen
         *
         * Replace the contents of the navigation stack with the specified screen. If an animation
         * is specified, the end result is the same as if the screen were pushed with the specified
         * animation.
         */
        inline static void Present(Screen *screen, const PresentationAnimation animation) {
            gShared->present(screen, animation);
        }

    private:
        ScreenManager();

        void draw();
        void drawScreen(Gfx::Framebuffer &fb, Screen *screen);

        void present(Screen *screen, const PresentationAnimation animation);

        void advanceAnimationFrame();
        void requestDraw();

    private:
        /// Global screen manager instance
        static ScreenManager *gShared;

        /// Maximum depth of navigation stack
        constexpr static const size_t kNavStackDepth{8};
        /// Navigation stack
        etl::stack<Screen *, kNavStackDepth> navStack;

        /// Animation period (in msec)
        constexpr static const size_t kAnimationPeriod{50};
        /// Timer used to drive animations
        TimerHandle_t animationTimer;
};
}

#endif
