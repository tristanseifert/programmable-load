/**
 * @file
 *
 * @brief Stack guard support
 *
 * Handler for stack guard corruption, definition of the actual stack cookie itself.
 */
#include <stdint.h>

/**
 * @brief Stack guard value
 *
 * The compiler-generated stack guard checks reference this variable to determine the correct value
 * to be placed in the stack frame; right now, this is a static value.
 *
 * @todo We should use the hardware RNG on bootup (before _any_ code really runs, so immediately
 * after reset) to randomize this.
 */
extern "C" uintptr_t __stack_chk_guard = 'lmao';

/**
 * Stack guard check failed
 */
extern "C"
[[noreturn]]
void __stack_chk_fail() {
    // TODO: implement
    asm volatile("bkpt 0xde" ::: "memory");
    while(1) {}
}
