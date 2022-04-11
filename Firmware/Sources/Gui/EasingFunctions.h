#ifndef GUI_EASINGFUNCTIONS_H
#define GUI_EASINGFUNCTIONS_H

#include <math.h>

namespace Gui {
/**
 * @brief Various easing functions
 *
 * See [easings.net](https://easings.net) for an interactive demonstration.
 */
struct EasingFunctions {
    constexpr static inline float InOutQuad(float t) {
        return t < 0.5f ? 2.f * t * t : t * (4.f - 2.f * t) - 1.f;
    }

    constexpr static inline float InOutCubic(float t) {
        float t2{t};
        return t2 < 0.5f ? 4.f * t2 * t2 * t2 : 1.f + (t2 - 1.f) * (2.f * (t2 - 2.f)) * (2.f * (t2 - 3.f));
    }

    constexpr static inline float InOutQuart(float t) {
        if(t < 0.5f) {
            t *= t;
            return 8.f * t * t;
        } else {
            --t;
            t = t * t;
            return 1.f - 8.f * t * t;
        }
    }

    constexpr static inline float InOutCirc(float t) {
        if(t < 0.5f) {
            return (1.f - sqrtf(1.f - 2.f * t)) * 0.5f;
        } else {
            return (1.f + sqrtf(2.f * t - 1.f)) * 0.5f;
        }
    }

    constexpr static inline float InOutElastic(float t) {
        float t2;
        if(t < 0.45f) {
            t2 = t * t;
            return 8.f * t2 * t2 * sinf(t * (float)M_PI * 9.f);
        } else if(t < 0.55f) {
            return 0.5f + 0.75f * sinf(t * (float)M_PI * 4.f);
        } else {
            t2 = (t - 1.f) * (t - 1.f);
            return 1.f - 8.f * t2 * t2 * sinf(t * (float)M_PI * 9.f);
        }
    }
};
}

#endif
