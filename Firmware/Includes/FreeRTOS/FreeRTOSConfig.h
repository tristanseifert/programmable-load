/**
 * @file FreeRTOS Configuration
 *
 * Defines some config variables for the FreeRTOS kernel. Most variables are left at their
 * defaults (undefined, kernel default) values.
 */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdint.h>

#include "stm32mp1xx.h"

extern void log_panic(const char *fmt, ...);

/// enable preemptive multithreading
#define configUSE_PREEMPTION                                    1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION                 1

/**
 * @brief Idle hook
 *
 * The idle hook is used to place the processor into a low power state until the next interrupt.
 */
#define configUSE_IDLE_HOOK                                     1
/// Disable tick hook
#define configUSE_TICK_HOOK                                     0

/**
 * @brief CPU core clock
 *
 * This is a variable exported by the startup code, containing the frequency of the current
 * system clock. It's filled in when the clocks are initialized.
 */
#define configCPU_CLOCK_HZ                                      (SystemCoreClock)
/**
 * @brief Ticks per second
 *
 * Defines the rate of timer interrupts per second, and in turn, the granularity of the software
 * timer facilities.
 */
#define configTICK_RATE_HZ                                      ((TickType_t) 1000)\

#define configMAX_PRIORITIES                                    (8)
#define configMINIMAL_STACK_SIZE                                ((unsigned short) 130)

#define configMAX_TASK_NAME_LEN                                 (16)
#define configUSE_TRACE_FACILITY                                1
#define configUSE_16_BIT_TICKS                                  0
#define configIDLE_SHOULD_YIELD                                 1
#define configUSE_MUTEXES                                       1
#define configQUEUE_REGISTRY_SIZE                               0

/**
 * @brief Stack high water mark setting
 *
 * Enable checking for task stack overflows
 */
#define configCHECK_FOR_STACK_OVERFLOW                          2
/// Enable recursive mutex
#define configUSE_RECURSIVE_MUTEXES                             1
/// Callback on failed malloc
#define configUSE_MALLOC_FAILED_HOOK                            1
#define configUSE_APPLICATION_TASK_TAG                          0
#define configUSE_COUNTING_SEMAPHORES                           1
#define configUSE_QUEUE_SETS                                    1
#define configGENERATE_RUN_TIME_STATS                           0

/// Disable coroutines
#define configUSE_CO_ROUTINES                                   0

/// Enable software timers
#define configUSE_TIMERS                                        1
// Run at middleware priority
#define configTIMER_TASK_PRIORITY                               (3)
#define configTIMER_QUEUE_LENGTH                                5
#define configTIMER_TASK_STACK_DEPTH                            (configMINIMAL_STACK_SIZE * 2)

/*
 * Enable direct-to-task notifications. Each task should get 4 notification values.
 */
#define configUSE_TASK_NOTIFICATIONS                            1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES                   4

/*
 * Enable thread local storage
 */
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS                 4

/// Enable static allocation support
#define configSUPPORT_STATIC_ALLOCATION                         1
/// Disable dynamic allocation support
#define configSUPPORT_DYNAMIC_ALLOCATION                        1

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */
#define INCLUDE_vTaskPrioritySet                                1
#define INCLUDE_uxTaskPriorityGet                               1
#define INCLUDE_vTaskDelete                                     1
#define INCLUDE_vTaskCleanUpResources                           1
#define INCLUDE_vTaskSuspend                                    1
#define INCLUDE_vTaskDelayUntil                                 1
#define INCLUDE_vTaskDelay                                      1
#define INCLUDE_eTaskGetState                                   1
#define INCLUDE_xTimerPendFunctionCall                          1

// include various functions for acquiring task handles
#define INCLUDE_xTaskGetCurrentTaskHandle                       1
#define INCLUDE_xTaskGetIdleTaskHandle                          1

// mutex functions to include
#define INCLUDE_xSemaphoreGetMutexHolder                        1

/* Cortex-M specific definitions. */
#ifdef __NVIC_PRIO_BITS
/* __BVIC_PRIO_BITS will be specified when CMSIS is being used. */
#define configPRIO_BITS                                 __NVIC_PRIO_BITS
#else
#error Define __NVIC_PRIO_BITS
#endif

/* The lowest interrupt priority that can be used in a call to a "set priority"
function. */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY                 0x0f

/* The highest interrupt priority that can be used by any interrupt service
routine that makes calls to interrupt safe FreeRTOS API functions.  DO NOT CALL
INTERRUPT SAFE FREERTOS API FUNCTIONS FROM ANY INTERRUPT THAT HAS A HIGHER
PRIORITY THAN THIS! (higher priorities are lower numeric values. */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY            5

/* Interrupt priorities used by the kernel port layer itself.  These are generic
to all Cortex-M ports, and do not rely on any particular library functions. */
#define configKERNEL_INTERRUPT_PRIORITY                         (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
/* !!!! configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to zero !!!!
See http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY                    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/**
 * @brief assert() style helper
 *
 * Same semantics as assert() from the C library; we should make this actually do stuff.
 */
#define configASSERT( x ) if( ( x ) == 0 ) { \
    taskDISABLE_INTERRUPTS();\
    log_panic("FreeRTOS assertion failure: %s (at %s:%u)", #x, __FILE__, __LINE__);\
    for( ;; );\
}\

/*
 * Map the FreeRTOS interrupt handler names to the CMSIS equivalents.
 */
/// SVC request handler
#define vPortSVCHandler SVC_Handler
/// PendSV handler
#define xPortPendSVHandler PendSV_Handler
/// SysTick IRQ
#define xPortSysTickHandler SysTick_Handler

#endif
