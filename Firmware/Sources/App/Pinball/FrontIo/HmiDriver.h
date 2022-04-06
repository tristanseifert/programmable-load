#ifndef APP_PINBALL_FROMTIO_HMIDRIVER_H
#define APP_PINBALL_FROMTIO_HMIDRIVER_H

#include <stddef.h>
#include <stdint.h>

#include "../FrontIoDriver.h"

#include "Drivers/I2CDevice/PCA9955B.h"
#include "Drivers/I2CDevice/XRA1203.h"

#include "Util/Uuid.h"

#include <etl/array.h>

namespace App::Pinball {
/**
 * @brief Driver for "Programmable load HMI"
 *
 * This implements the driver for the front panel IO board in the `Hardware` directory, featuring
 * an XRA1203 IO expander for buttons, and a PCA9955B LED driver. There is also a rotary encoder,
 * but the signals are passed to the processor board to handle.
 *
 * TODO: read LED currents from IDPROM
 */
class HmiDriver: public FrontIoDriver {
    public:
        /// Driver UUID bytes
        constexpr static const etl::array<uint8_t, Util::Uuid::kByteSize> kUuidBytes{{
            0xde, 0xf5, 0x21, 0x2a, 0x92, 0x76, 0x47, 0xd7, 0x93, 0xb4, 0x5e, 0x25, 0x52, 0x6a, 0x8c, 0x95
        }};

        /**
         * @brief Driver UUID
         *
         * Use this UUID in the inventory ROM on the front panel to match the driver.
         */
        static const Util::Uuid kDriverId;

    public:
        HmiDriver(Drivers::I2CBus *bus, Drivers::I2CDevice::AT24CS32 &idprom);
        ~HmiDriver() override;

        void handleIrq() override;
        int setIndicatorState(const Indicator state) override;

    private:
        /// Button IO expander
        //Drivers::I2CDevice::XRA1203 ioExpander;
        /// LED driver
        //Drivers::I2CDevice::PCA9955B ledDriver;
};
}

#endif
