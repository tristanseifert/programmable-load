#include <metal/io.h>
#include <metal/sys.h>
#include <metal/utilities.h>
#include <stdint.h>

/**
 * @brief Flush cache covering the given address range
 *
 * This is a no-op as the M4 core doesn't have any cache.
 */
void metal_machine_cache_flush(void *addr, unsigned int len) {
    (void) addr;
    (void) len;
}

/**
 * @brief Invalidate cache covering the given address range
 *
 * This is a no-op as the M4 core doesn't have any cache.
 */
void metal_machine_cache_invalidate(void *addr, unsigned int len) {
    (void) addr;
    (void) len;
}

/**
 * @brief Add an MMIO mapping
 *
 * Since we don't have an MMU (and the MPU is unused) this just returns the requested address
 * directly.
 */
void *metal_machine_io_mem_map(void *va, metal_phys_addr_t pa, size_t size, unsigned int flags) {
    (void) pa;
    (void) size;
    (void) flags;

    return va;
}
