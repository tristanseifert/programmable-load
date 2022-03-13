#include "Logger.h"
#include "TraceSWO.h"

#include <printf/printf.h>

#include <stdarg.h>

using namespace Log;

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
        // send to SWO
            TraceSWO::PutChar(c);
    }, nullptr, format.data(), args);
#pragma clang diagnostic pop
}

