#ifndef LOG_TRACESWO_H
#define LOG_TRACESWO_H

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

        static void Init();
        static void PutChar(const char ch);
};
}

#endif
