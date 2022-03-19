/**
 * @file
 *
 * @brief Idle handler
 *
 * Implements the idle callback, which in turn is used to place the processor into a lower power
 * state.
 */
#include "Rtos.h"

#include <vendor/sam.h>

using namespace Rtos;

/**
 * @brief Idle hook
 *
 * Called by FreeRTOS when the idle task gets scheduled. It'll place the processor into a low
 * power state, until the next interrupt.
 */
extern "C" void vApplicationIdleHook() {
    __WFI();
}
