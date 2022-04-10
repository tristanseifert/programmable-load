/**
 * @file
 *
 * @brief Trigonometric functions implementation
 *
 * Provides a basic implementation of sine, cosine and tangent.
 */

#include <math.h>
#include <arm_math.h>

/**
 * @brief Calculate sine
 *
 * Thunk through to the CMSIS DSP library.
 *
 * @param x Angle, in radians
 */
float sinf(float x) {
    return arm_sin_f32(x);
}

/**
 * @brief Calculate cosine
 *
 * Thunk through to the CMSIS DSP library.
 *
 * @param x Angle, in radians
 */
float cosf(float x) {
    return arm_cos_f32(x);
}
