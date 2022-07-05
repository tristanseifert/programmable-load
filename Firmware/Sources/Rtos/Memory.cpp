/**
 * @file FreeRTOS memory allocation support
 *
 * Implements callbacks to get static memory for FreeRTOS tasks
 */
#include "Rtos.h"
#include "Log/Logger.h"

#include <stddef.h>
#include <stdint.h>

namespace Rtos {

/// TCB for the idle task
static StaticTask_t gIdleTcb;
/// Size of the idle task's stack, in words
constexpr static const size_t kIdleStackSize{configMINIMAL_STACK_SIZE};
/// Stack for the idle task
static StackType_t gIdleStack[kIdleStackSize];

/// TCB for the timer task
static StaticTask_t gTimerTcb;
/// Size of the idle task's stack, in words
constexpr static const size_t kTimerStackSize{configTIMER_TASK_STACK_DEPTH};
/// Stack for the timer task
static StackType_t gTimerStack[kTimerStackSize];



/**
 * @brief Task stack overflow hook
 *
 * Invoked when a task's stack exceeds the valid high water mark; this is checked by setting the
 * top 16 bytes of the stack to a known value, and doing a comparison.
 */
extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask,
        char *taskName) {
    Logger::Panic("Stack overflow (task '%s')", taskName);
}

/**
 * @brief Provide memory for idle task
 *
 * To support static memory allocation, the kernel invokes this function to get the location of the
 * memory in .bss for the idle task structure and its stack.
 *
 * @param outTcb Will receive the address of the preallocated TCB
 * @param outStack Will receive address of the stack
 * @param outStackSize Will receive size of the stack, in words
 */
extern "C" void vApplicationGetIdleTaskMemory(StaticTask_t **outTcb, StackType_t **outStack,
                                   uint32_t *outStackSize) {
    *outTcb = &gIdleTcb;
    *outStack = gIdleStack;
    *outStackSize = kIdleStackSize;
}

/**
 * @brief Provide memory for timer task
 *
 * Returns the TCB and stack allocated in .bss for the timer task.
 *
 * @param outTcb Will receive the address of the preallocated TCB
 * @param outStack Will receive address of the stack
 * @param outStackSize Will receive size of the stack, in words
 */
extern "C" void vApplicationGetTimerTaskMemory(StaticTask_t **outTcb, StackType_t **outStack,
                                    uint32_t *outStackSize) {
    *outTcb = &gTimerTcb;
    *outStack = gTimerStack;
    *outStackSize = kTimerStackSize;
}



/**
 * @brief malloc failure hook
 *
 * Invoked when a FreeRTOS memory allocation fails.
 */
extern "C" void vApplicationMallocFailedHook() {
    Logger::Panic("out of memory (heap)");
}
}
