#include "Log/Logger.h"

#include <stddef.h>

void operator delete(void* p) {
    Logger::Panic("delete called on %p", p);
}

// Same as above, just a C++14 specialization.
// (See http://en.cppreference.com/w/cpp/memory/new/operator_delete)
void operator delete(void* p, size_t t) {
    Logger::Panic("delete called on %p (%zu)", p, t);
}
