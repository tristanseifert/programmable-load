#include "Start.h"
#include "Log/Logger.h"

#include <FreeRTOS.h>
#include <task.h>

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
