/**
 * @file
 *
 * @brief Heap initialization
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <umm_malloc.h>

extern uint8_t _sheap, _eheap;

/**
 * @brief Initialize the heap
 *
 * This sets up the system's heap based on the _sheap and _eheap symbols exported by the linker
 * script.
 */
void __libc_heap_init() {
    const size_t heapBytes = ((uintptr_t) &_eheap) - ((uintptr_t) &_sheap);

    // first, zero the heap area
    memset(&_sheap, 0, heapBytes);

    // then initialize it
    umm_init_heap(&_sheap, heapBytes);
}
