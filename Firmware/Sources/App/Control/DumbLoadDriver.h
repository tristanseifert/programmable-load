#ifndef APP_CONTROL_DUMBLOADDRIVER_H
#define APP_CONTROL_DUMBLOADDRIVER_H

#include "LoadDriver.h"

namespace App::Control {
/**
 * @brief Driver for "dumb" analog board
 *
 * This is a driver for the basic programmable load board, which features a pile of discrete ADCs
 * and DACs to implement the load control. The control loop is implemented in software in the
 * firmware for modes other than constant current.
 *
 * In addition, the driver implements support for remote voltage sense (under automatic control)
 * and temperature reporting of the MOSFETs. The number of MOSFET channels is also configurable
 * via the PROM, to disable the second channel if needed. Lastly, there is an EMC2101 fan 
 * controller on the board, which is used for automatic control of the fan cooling the MOSFETs
 * with a discrete temperature sensor connected to the driver.
 *
 * Calibratio values (for the input voltage sense, current sense, and current driver) are read
 * from the IDPROM during initialization and applied.
 */
class DumbLoadDriver: public LoadDriver {
    public:
        DumbLoadDriver(Drivers::I2CBus *bus, Drivers::I2CDevice::AT24CS32 &idprom);
        ~DumbLoadDriver();

};
}

#endif
