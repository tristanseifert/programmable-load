#ifndef APP_THERMAL_HARDWARE
#define APP_THERMAL_HARDWARE

#include <stdint.h>

namespace Drivers {
class I2CBus;

namespace I2CDevice {
class EMC2101;
}
}

namespace App::Thermal {
/**
 * @brief On-board thermal management hardware
 *
 * This mostly consists of an EMC2101 on the processor board, which controls the case rear fan. It
 * also sets up measurement of the processor's on-board temperature sensor.
 */
class Hw {
    friend class Task;

    public:
        static void InitFanController(Drivers::I2CBus *bus);

    private:
        /**
         * @brief On-board fan controller
         *
         * It's intended to drive a case rear fan.
         */
        static Drivers::I2CDevice::EMC2101 *gFanController;
};
}

#endif
