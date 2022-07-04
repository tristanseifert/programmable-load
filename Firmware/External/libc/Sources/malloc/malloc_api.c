/**
 * @file
 *
 * @brief C library memory allocation wrappers
 *
 * Provides the standard C library malloc/realloc/free interface, as thin wrappers around the
 * internal memory allocator.
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <umm_malloc.h>

void *malloc(size_t numBytes) {
    return umm_malloc(numBytes);
}

void *calloc(size_t count, size_t numBytes) {
    return umm_calloc(count, numBytes);
}

void *realloc(void *ptr, size_t newNumBytes) {
    return umm_realloc(ptr, newNumBytes);
}

void free(void *ptr) {
    return umm_free(ptr);
}


