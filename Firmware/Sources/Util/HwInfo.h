#ifndef UTIL_HWINFO_H
#define UTIL_HWINFO_H

#include <stddef.h>
#include <stdint.h>

namespace Util {
/**
 * @brief Information about the hardware
 *
 * This exposes information about the hardware we're running on, such as its revision, type,
 * and serial number.
 */
class HwInfo {
    public:
        static void Init();

        /**
         * @brief Get hardware revision
         */
        static inline auto GetRevision() {
            return gRevision;
        }
        /**
         * @brief Get serial number
         */
        static inline auto GetSerial() {
            return gSerial;
        }

    private:
        static uint16_t gRevision;
        static const char *gSerial;
};
}

#endif
