#ifndef DRIVERS_RANDOM_H
#define DRIVERS_RANDOM_H

#include <stddef.h>
#include <stdint.h>

namespace Drivers {
/**
 * @brief Random number generator
 *
 * Provides an interface to the on-board "true" random number generator.
 *
 * @TODO: Figure out how to get Linux to un-gate the PLL4R clock - it's not running, hence the
 *        need to manually change clock sources.
 */
class Random {
    public:
        static void Init();

        static uint32_t Get();

    private:
        /// Timeout (in cycles) for the RNG to become ready after initialization
        constexpr static const size_t kInitTimeout{1'000'000};
        /// Timeout (in cycles) for the RNG to produce data
        constexpr static const size_t kRefillTimeout{10'000};

};
}

#endif
