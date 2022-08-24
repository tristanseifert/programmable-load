#ifndef LOG_LOGGER_H
#define LOG_LOGGER_H

#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include <etl/string_view.h>

extern "C" void log_panic(const char *, ...);

/// Log message handling
namespace Log {
/**
 * @brief Global logging handler
 *
 * The logger is a global object capable of formatting messages, at a given intensity level, and
 * writing them to multiple output destinations. Logs may also be archived on some form of
 * persistent storage for later retrieval.
 */
class Logger {
    friend void ::log_panic(const char *, ...);

    public:
        /**
         * @brief Size of a per task log buffer (in bytes)
         *
         * This sets an upper cap on the maximum length of a single log message.
         */
        constexpr static const size_t kTaskLogBufferSize{256};

        /// Size of the trace buffer (in bytes)
        constexpr static const size_t kTraceBufferSize{0x1000};

        /**
         * @brief Trace logging buffer
         *
         * This is a circular buffer that receives all log messages in the system. We'll start
         * writing at the top, and continue til the end, then loop around.
         *
         * Its size is fixed at compile time, as it is exposed to the host via the resource table
         * mechanism.
         *
         * @remark This is only accessible here so the resource table can know its location; you
         * should not manually interact with it.
         */
        static char gTraceBuffer[kTraceBufferSize];

    private:
        /// Write pointer into the trace buffer
        static size_t gTraceWritePtr;

        /// Put a character into the trace buffer
        static inline void TracePutChar(const char ch) {
            gTraceBuffer[gTraceWritePtr] = ch;
            // handle wrap-around here
            if(++gTraceWritePtr >= kTraceBufferSize) {
                gTraceWritePtr = 0;
            }
        }
        /**
         * @brief Write a string into the trace buffer
         *
         * This ensures the string is written in one continuous go. This means we may loop back to
         * the start of the buffer (and then clear all subsequent string data out) if there's not
         * enough space at the end.
         *
         * Strings are automatically terminated with a newline to delimit messages.
         *
         * @remark This method is not thread safe.
         */
        static inline void TracePutString(const char *str, const size_t numChars) {
            // check if there's sufficient space
            const auto bytesFree = kTraceBufferSize - gTraceWritePtr;
            const auto bytesNeeded = numChars + 1; // plus newline

            // there isn't, so write the string at the start
            if(bytesFree < bytesNeeded) {
                memcpy(gTraceBuffer, str, numChars);

                // reset write ptr and add newline
                gTraceWritePtr = numChars;
                gTraceBuffer[gTraceWritePtr++] = '\n';
            }
            // otherwise, write it at the current position
            else {
                memcpy((gTraceBuffer + gTraceWritePtr), str, numChars);

                // advance write ptr and add newline
                gTraceWritePtr += numChars;
                gTraceBuffer[gTraceWritePtr++] = '\n';
            }

            // kill any remaining partial message on the next line
            size_t ptr{gTraceWritePtr};

            while(ptr < kTraceBufferSize &&
                    (gTraceBuffer[ptr] != '\n' || gTraceBuffer[ptr] != '\0')) {
                gTraceBuffer[ptr++] = '\0';
            }
        }

    public:
        /**
         * @brief Log level
         *
         * An enumeration defining the different log levels (intensities) available. Messages with
         * a level below the cutoff may be filtered out and discarded.
         */
        enum class Level: uint8_t {
            /// Most severe type of error
            Error                       = 5,
            /// A significant problem in the system
            Warning                     = 4,
            /// General information
            Notice                      = 3,
            /// Bonus debugging information
            Debug                       = 2,
            /// Even more verbose debugging information
            Trace                       = 1,
        };

    public:
        /// You cannot create logger instances; there is just one shared boi
        Logger() = delete;

        /**
         * @brief Panic the system
         *
         * This is the same as logging an error and then invoking halt callback.
         *
         * @param fmt Format string
         * @param ... Arguments to message
         */
        [[noreturn]] static void Panic(const etl::string_view fmt, ...) {
            va_list va;
            va_start(va, fmt);
            Log(Level::Error, fmt, va);
            va_end(va);

            Panic();
        }

        /**
         * @brief Output an error level message.
         *
         * @param fmt Format string
         * @param ... Arguments to message
         */
        static void Error(const etl::string_view fmt, ...) {
            va_list va;
            va_start(va, fmt);
            Log(Level::Error, fmt, va);
            va_end(va);
        }

        /**
         * @brief Output a warning level message.
         *
         * @param fmt Format string
         * @param ... Arguments to message
         */
        static void Warning(const etl::string_view fmt, ...) {
            if(static_cast<uint8_t>(gLevel) > static_cast<uint8_t>(Level::Warning)) return;

            va_list va;
            va_start(va, fmt);
            Log(Level::Warning, fmt, va);
            va_end(va);
        }

        /**
         * @brief Output a notice level message.
         *
         * @param fmt Format string
         * @param ... Arguments to message
         */
        static void Notice(const etl::string_view fmt, ...) {
            if(static_cast<uint8_t>(gLevel) > static_cast<uint8_t>(Level::Notice)) return;

            va_list va;
            va_start(va, fmt);
            Log(Level::Notice, fmt, va);
            va_end(va);
        }

        /**
         * @brief Output a debug level message.
         *
         * @param fmt Format string
         * @param ... Arguments to message
         */
        static void Debug(const etl::string_view fmt, ...) {
            if(static_cast<uint8_t>(gLevel) > static_cast<uint8_t>(Level::Debug)) return;

            va_list va;
            va_start(va, fmt);
            Log(Level::Debug, fmt, va);
            va_end(va);
        }

        /**
         * @brief Output a trace level message.
         *
         * @param fmt Format string
         * @param ... Arguments to message
         */
        static void Trace(const etl::string_view fmt, ...) {
            if(static_cast<uint8_t>(gLevel) > static_cast<uint8_t>(Level::Trace)) return;

            va_list va;
            va_start(va, fmt);
            Log(Level::Trace, fmt, va);
            va_end(va);
        }

        static void Log(const Level lvl, const etl::string_view &fmt, va_list args);

    private:
        [[noreturn]] static void Panic();

    private:
        static bool gInitialized;
        static Level gLevel;
};
}

using Logger = Log::Logger;

#define REQUIRE(cond, ...) {if(!(cond)) { Logger::Panic(__VA_ARGS__); }}

#endif
