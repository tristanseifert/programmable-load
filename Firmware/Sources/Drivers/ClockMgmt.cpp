#include "ClockMgmt.h"

#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include <vendor/sam.h>

using namespace Drivers;

/**
 * @brief Enable a peripheral clock
 *
 * Configures the peripheral channel clock, to draw its clock from a particular source.
 *
 * @param periph Peripheral channel to configure, then enable
 * @param source Clock source to associate
 */
void ClockMgmt::EnableClock(const Peripheral periph, const Clock source) {
    GCLK->PCHCTRL[static_cast<size_t>(periph)].reg = GCLK_PCHCTRL_CHEN |
        GCLK_PCHCTRL_GEN(static_cast<uint32_t>(source));
}

