#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include <stddef.h>

void *operator new(size_t numBytes) {
    return pvPortMalloc(numBytes);
}

void operator delete(void* p) {
    vPortFree(p);
}

// Same as above, just a C++14 specialization.
// (See http://en.cppreference.com/w/cpp/memory/new/operator_delete)
void operator delete(void* p, size_t t) {
    vPortFree(p);
}
