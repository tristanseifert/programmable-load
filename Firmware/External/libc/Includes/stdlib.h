/**
 * @file
 *
 * @brief C standard library functions
 */
#ifndef STDLIB_H
#define STDLIB_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void abort() __attribute__((noreturn));

void *malloc(size_t);
void *calloc(size_t, size_t);
void *realloc(void *, size_t);
void free(void *);

#ifdef __cplusplus
}
#endif

#endif
