/**
 * @file
 *
 * @brief RTOS helpers
 *
 * Various definitions (and umbrella include) for working with FreeRTOS
 */
#ifndef RTOS_RTOS_H
#define RTOS_RTOS_H

// pull in all the FreeRTOS stuff
#include <FreeRTOS.h>
#include <task.h>

namespace Rtos {
/**
 * @brief Firmware-specific priority level assignments
 *
 * Each entry in this enum defines the priority value for a particular "class" of tasks. This
 * ensures that important processing cannot get starved out by less important stuff.
 */
enum TaskPriority: UBaseType_t {
    /**
     * @brief Deferred interrupt calls
     */
    Dpc                                 = (configMAX_PRIORITIES - 1),
    /**
     * @brief Driver work loops
     */
    Driver                              = (Dpc - 1),
    /**
     * @brief Middleware
     *
     * This includes stuff such as high-level protocol drivers (such as USB or network protocol
     * handlers) and timers.
     */
    Middleware                          = (Driver - 1),
    /**
     * @brief High priority app
     *
     * Application tasks that have a relatively higher priority, such as control loops.
     */
    AppHigh                             = (Middleware - 1),
    /**
     * @brief Low priority app
     *
     * Low priority application tasks, such as user interfaces.
     */
    AppLow                              = (AppHigh - 1),
    /**
     * @brief Idle
     *
     * Tasks that run when no other processing in the system is going on; useful for background
     * maintenance type tasks.
     */
    Background                          = (AppLow - 1),
};
static_assert(TaskPriority::Background >= 0);
static_assert(TaskPriority::Background < configMAX_PRIORITIES);

/**
 * @brief Task notification indices
 *
 * System-wide reserved indices in the task notification array
 */
enum TaskNotifyIndex: size_t {
    /// reserved for FreeRTOS message buffer api
    Stream                              = 0,
    /// First task specific value
    TaskSpecific                        = 1,
};
}

#endif
