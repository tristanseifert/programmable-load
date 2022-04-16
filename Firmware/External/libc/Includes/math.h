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

#ifndef HUGE_VAL
#  define HUGE_VAL (__builtin_huge_val())
#endif

#ifndef HUGE_VALF
#  define HUGE_VALF (__builtin_huge_valf())
#endif

#ifndef HUGE_VALL
#  define HUGE_VALL (__builtin_huge_vall())
#endif

#ifndef INFINITY
#  define INFINITY (__builtin_inff())
#endif

#ifndef NAN
#  define NAN (__builtin_nanf(""))
#endif

/* fpclassify() and friends */
#define FP_NAN         0
#define FP_INFINITE    1
#define FP_ZERO        2
#define FP_SUBNORMAL   3
#define FP_NORMAL      4


extern int __isinff (float x);
extern int __isinfd (double x);
extern int __isnanf (float x);
extern int __isnand (double x);
extern int __fpclassifyf (float x);
extern int __fpclassifyd (double x);
extern int __signbitf (float x);
extern int __signbitd (double x);

#define fpclassify(__x) \
	((sizeof(__x) == sizeof(float))  ? __fpclassifyf(__x) : \
	__fpclassifyd(__x))

#ifndef isfinite
  #define isfinite(__y) \
          (__extension__ ({int __cy = fpclassify(__y); \
                           __cy != FP_INFINITE && __cy != FP_NAN;}))
#endif

/* Note: isinf and isnan were once functions in newlib that took double
 *       arguments.  C99 specifies that these names are reserved for macros
 *       supporting multiple floating point types.  Thus, they are
 *       now defined as macros.  Implementations of the old functions
 *       taking double arguments still exist for compatibility purposes
 *       (prototypes for them are in <ieeefp.h>).  */
#ifndef isinf
  #define isinf(y) (fpclassify(y) == FP_INFINITE)
#endif

#ifndef isnan
  #define isnan(y) (fpclassify(y) == FP_NAN)
#endif

#define isnormal(y) (fpclassify(y) == FP_NORMAL)
#define signbit(__x) \
	((sizeof(__x) == sizeof(float))  ?  __signbitf(__x) : \
		__signbitd(__x))

#define isgreater(x,y) \
          (__extension__ ({__typeof__(x) __x = (x); __typeof__(y) __y = (y); \
                           !isunordered(__x,__y) && (__x > __y);}))
#define isgreaterequal(x,y) \
          (__extension__ ({__typeof__(x) __x = (x); __typeof__(y) __y = (y); \
                           !isunordered(__x,__y) && (__x >= __y);}))
#define isless(x,y) \
          (__extension__ ({__typeof__(x) __x = (x); __typeof__(y) __y = (y); \
                           !isunordered(__x,__y) && (__x < __y);}))
#define islessequal(x,y) \
          (__extension__ ({__typeof__(x) __x = (x); __typeof__(y) __y = (y); \
                           !isunordered(__x,__y) && (__x <= __y);}))
#define islessgreater(x,y) \
          (__extension__ ({__typeof__(x) __x = (x); __typeof__(y) __y = (y); \
                           !isunordered(__x,__y) && (__x < __y || __x > __y);}))

#define isunordered(a,b) \
          (__extension__ ({__typeof__(a) __a = (a); __typeof__(b) __b = (b); \
                           fpclassify(__a) == FP_NAN || fpclassify(__b) == FP_NAN;}))

#ifdef __cplusplus
}
#endif

#endif
