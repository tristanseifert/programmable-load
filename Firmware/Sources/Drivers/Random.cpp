#include "Random.h"

#include "Log/Logger.h"

#include <vendor/sam.h>

using namespace Drivers;

/**
 * @brief Initialize random number generator
 *
 * This enables the clock and reset the TRNG.
 */
void Random::Init() {
    // enable clocks
    MCLK->APBCMASK.bit.TRNG_ = true;

    // enable RNG
    TRNG->CTRLA.reg = TRNG_CTRLA_ENABLE;
}

uint32_t Random::Get() {
    while(!TRNG->INTFLAG.bit.DATARDY) {}
    return TRNG->DATA.reg;
}

