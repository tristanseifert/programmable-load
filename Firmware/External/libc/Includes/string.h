/**
 * @file
 *
 * @brief String helper functions
 *
 * This file defines functions traditionally exported by a C library in string.h.
 */
#ifndef STRING_H
#define STRING_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *memcpy(void *dst0, const void *src0, size_t length);

#ifdef __cplusplus
}
#endif

#endif
