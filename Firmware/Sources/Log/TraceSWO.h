#ifndef LOG_TRACESWO_H
#define LOG_TRACESWO_H

#include <stdint.h>

namespace Log {
/**
 * @brief Log output to trace SWO mechanism
 *
 * This is a backend that dumps characters out through the SWD debug port's SWO (serial wire out)
 * facility.
 */
class TraceSWO {
    public:
        TraceSWO() = delete;

        static void Init(const uint32_t freq);
        static void PutChar(const char ch);
};
}

#endif
