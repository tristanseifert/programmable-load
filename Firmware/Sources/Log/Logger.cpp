#include "Logger.h"

#include "Rtos/Rtos.h"

#include <printf/printf.h>

#include <stdarg.h>

#include "stm32mp1xx.h"
#include "Hw/StatusLed.h"

#include <etl/array.h>

using namespace Log;

char Logger::gTraceBuffer[kTraceBufferSize];
size_t Logger::gTraceWritePtr{0};

/**
 * @brief Indicates whether the logger backends have been initialized
 *
 * During early system boot, we won't have any output backends for log messages set up; in this
 * case, fall back to just printing them out the swd trace line.
 */
bool Logger::gInitialized{false};

/**
 * @brief Log level
 *
 * All log messages with a level that is numerically lower than this one will be discarded.
 */
Logger::Level Logger::gLevel{Logger::Level::Trace};



/**
 * @brief Output a log message.
 *
 * @param level Message level
 * @param format Format string, with printf-style substitutions
 * @param args Arguments to format
 */
void Logger::Log(const Level level, const etl::string_view &format, va_list args) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
    vfctprintf([](char c, auto) {
        // TODO: other output mechanisms

        // write to trace buffer
        TracePutChar(c);
    }, nullptr, format.data(), args);

    // terminate the message in the trace buffer
    TracePutChar('\n');
#pragma clang diagnostic pop
}

/**
 * @brief Panic the system
 *
 * This disables interrupts and lands ourselves into an infinite loop and/or breakpoint.
 */
void Logger::Panic() {
    // print a message
    Error("Panic! at the system, halting");
    Hw::StatusLed::Set(Hw::StatusLed::Color::Red);

    // get task info (if scheduler is running)
    unsigned long totalRuntime{0};
    constexpr static const size_t kTaskInfoSize{8};
    static etl::array<TaskStatus_t, kTaskInfoSize> gTaskInfo;

    if(xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        //const auto ok = uxTaskGetSystemState(gTaskInfo.data(), kTaskInfoSize, &totalRuntime);
        const auto ok = uxTaskGetSystemState(gTaskInfo.data(), kTaskInfoSize, nullptr);

        if(!ok) {
            Error("Failed to get RTOS state");
        } else {
            Error("========== RTOS state ==========");
            Error("Total runtime: %10lu", totalRuntime);
            Error("%8s %-16s S %10s %2s %3s", "Handle", "Name", "Runtime", "PR", "STK");

            for(size_t i = 0; i < ok; i++) {
                const auto &task = gTaskInfo[i];
                char stateChar{'?'};

                switch(task.eCurrentState) {
                    case eReady:
                        stateChar = 'R';
                        break;
                    case eRunning:
                        stateChar = '*';
                        break;
                    case eBlocked:
                        stateChar = 'B';
                        break;
                    case eSuspended:
                        stateChar = 'S';
                        break;
                    case eDeleted:
                        stateChar = 'x';
                        break;
                    default:
                        break;
                }

                Error("%08x %-16s %c %10lu %2u %03x", task.xHandle, task.pcTaskName, stateChar,
                        task.ulRunTimeCounter, task.uxCurrentPriority, task.usStackHighWaterMark);
            }
        }
    }

    // stop machine
    __disable_irq();
    __BKPT(0xf3);

    while(1) {}
}

/**
 * @brief C panic function
 *
 * Mimics a real panic from a C only source file (such as FreeRTOS)
 *
 * @param fmt Format string
 * @param ... Arguments to format (in format string)
 */
extern "C" void log_panic(const char *fmt, ...) {
    using Level = Log::Logger::Level;

    va_list va;
    va_start(va, fmt);
    Log::Logger::Log(Level::Error, fmt, va);
    va_end(va);

    Logger::Panic();
}

/**
 * @brief C logging thunk
 *
 * Calls through to the actual logging implementation from C.
 *
 * @param inLevel Message level to output at; equal to one of the constant values of Level enum
 * @param fmt Format string
 * @param ... Arguments to format (in format string)
 */
extern "C" void do_log(const unsigned int inLevel, const char *fmt, ...) {
    using Level = Log::Logger::Level;

    va_list va;
    va_start(va, fmt);

    // validate priority
    Level lvl;
    switch(inLevel) {
        case 5:
            lvl = Level::Error;
            break;
        case 4:
            lvl = Level::Warning;
            break;
        case 3:
            lvl = Level::Notice;
            break;
        case 2:
            lvl = Level::Debug;
            break;
        case 1:
            lvl = Level::Trace;
            break;

        // unknown log level
        default:
            Logger::Error("Invalid log level: %u", inLevel);
            return;
    }

    // perform logging
    Log::Logger::Log(lvl, fmt, va);
    va_end(va);
}

