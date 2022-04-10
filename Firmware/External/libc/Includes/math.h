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
    if(x == 0.f) return 0.f; // deals with both 0.0 and -0.0.
    if(x > 0.f) return x; // deals with ]0.0 .. +inf], but "(NaN > 0.0)" is false
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

float sinf(float x);
float cosf(float x);

#define MAXFLOAT	3.40282347e+38F

#define M_E		2.7182818284590452354
#define M_LOG2E		1.4426950408889634074
#define M_LOG10E	0.43429448190325182765
#define M_LN2		_M_LN2
#define M_LN10		2.30258509299404568402
#define M_PI		3.14159265358979323846
#define M_PI_2		1.57079632679489661923
#define M_PI_4		0.78539816339744830962
#define M_1_PI		0.31830988618379067154
#define M_2_PI		0.63661977236758134308
#define M_2_SQRTPI	1.12837916709551257390
#define M_SQRT2		1.41421356237309504880
#define M_SQRT1_2	0.70710678118654752440

#ifdef __cplusplus
}
#endif

#endif
