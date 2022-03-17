/**
 * @file
 *
 * @brief C runtime assertion support
 */
#ifndef ASSERT_H
#define ASSERT_H

#ifdef NDEBUG
#define assert(EX)
#else
#define assert(EX) (void)((EX) || (__AssertHandler(#EX, __FILE__, __LINE__),0))
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern void __AssertHandler(const char *expr, const char *file, int line);

#ifdef __cplusplus
}
#endif

#endif
