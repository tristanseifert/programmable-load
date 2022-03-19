#include "Start.h"
#include "Rtos.h"
#include "Log/Logger.h"

using namespace Rtos;

/**
 * @brief Start the RTOS scheduler
 */
[[noreturn]] void Rtos::StartScheduler() {
    vTaskStartScheduler();

    // XXX: should never get here
    asm volatile("bkpt 0xf3" ::: "memory");
    while(1) {}
}
