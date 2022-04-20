#ifndef DRIVERS_RANDOM_H
#define DRIVERS_RANDOM_H

#include <stdint.h>

namespace Drivers {
/**
 * @brief Random number generator
 *
 * Provides an interface to the on-board "true" random number generator.
 */
class Random {
    public:
        static void Init();

        static uint32_t Get();
};
}

#endif
