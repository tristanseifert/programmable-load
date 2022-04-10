/**
 * @file
 *
 * @brief Math routines
 */
#ifndef MATH_H
#define MATH_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Return the absolute value of an integer.
 */
static inline int abs(int x) {
    if(!x) return 0;
    else if(x > 0) return x;
    else return -x;
}

/**
 * @brief Return the absolute value of a floating point number.
 */
static inline float fabsf(float x) {
    if(x == 0.0) return 0.0; // deals with both 0.0 and -0.0.
    if(x > 0.0) return x; // deals with ]0.0 .. +inf], but "(NaN > 0.0)" is false
    return -x; // deals with [-inf .. 0.0[, and NaNs
}

/**
 * @brief Calculate floating point square root
 */
static inline float sqrtf(float x) {
    float result;
    asm("vsqrt.f32 %0, %1" : "=w" (result) : "w" (x));
    return result;
}

#ifdef __cplusplus
}
#endif

#endif
