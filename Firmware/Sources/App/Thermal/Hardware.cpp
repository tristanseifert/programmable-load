#include "Hardware.h"

#include "Drivers/ExternalIrq.h"
#include "Drivers/Gpio.h"
#include "Drivers/I2CBus.h"
#include "Drivers/I2CDevice/EMC2101.h"

#include "Log/Logger.h"

using namespace App::Thermal;

Drivers::I2CDevice::EMC2101 *Hw::gFanController{nullptr};

/**
 * @brief Initialize the on-board fan controller
 *
 * An EMC2101 is used on the processor board to control the fan in the rear of the case. We operate
 * it in manual control mode, where the thermal management task manually sets the fan speed.
 *
 * @param bus The I²C bus to which the controller is connected
 */
void Hw::InitFanController(Drivers::I2CBus *bus) {
    using Drivers::I2CDevice::EMC2101;

    static const EMC2101::Config cfg{
        // control fan with PWM
        .analogFan = 0,
        // enable tachometer input
        .tach = 1,
        // minimum fan speed: 500 RPM
        .minRpm = 500,
    };

    static uint8_t gBuf[sizeof(EMC2101)] __attribute__((aligned(alignof(EMC2101))));
    auto ptr = reinterpret_cast<EMC2101 *>(gBuf);
    gFanController = new (ptr) EMC2101(bus, cfg);

    // apply configuration
    gFanController->setFanMode(EMC2101::FanMode::Manual);
}
