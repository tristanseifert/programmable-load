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

int memcmp(const void *s1, const void *s2, size_t n);
void *memcpy(void *dst0, const void *src0, size_t length);
void *memset(void *b, int c, size_t len);

char *strchr(const char *p, int ch);
int strcmp(const char *s1, const char *s2);
size_t strcspn(const char *s1, const char *s2);
size_t strspn(const char *s1, const char *s2);

/*
 * Unsafe string functions
 *
 * These are functions that can be extremely vulnerable to buffer overflows or other similar
 * issues. You need to define `WITH_SCARY_FUNCTIONS` for their declarations to be available in
 * these headers.
 */
#ifdef WITH_SCARY_FUNCTIONS

char *strcpy(char *to, const char *from);
size_t strlen(const char *str);

#endif

#ifdef __cplusplus
}
#endif

#endif
