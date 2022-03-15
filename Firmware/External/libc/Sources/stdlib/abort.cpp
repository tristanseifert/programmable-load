#include "stdlib.h"

/**
 * Handles abnormal program conditions by breaking into the debugger.
 */
void abort() {
    asm volatile("bkpt 0xf0" ::: "memory");
    while(1) {}
}
