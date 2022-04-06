#include "HmiDriver.h"

#include "Drivers/I2CBus.h"
#include "Drivers/I2CDevice/AT24CS32.h"
#include "Log/Logger.h"
#include "Util/InventoryRom.h"
#include "Rtos/Rtos.h"

using namespace App::Pinball;

const Util::Uuid HmiDriver::kDriverId(kUuidBytes);

/**
 * @brief Initialize the HMI
 *
 * This sets up the IO expander and LED driver at their default addresses.
 */
HmiDriver::HmiDriver(Drivers::I2CBus *bus, Drivers::I2CDevice::AT24CS32 &idprom) : 
    FrontIoDriver(bus, idprom), 
    //ioExpander(Drivers::I2CDevice::XRA1203(bus, kExpanderAddress, kPinConfigs)),
    ledDriver(Drivers::I2CDevice::PCA9955B(bus, kLedDriverAddress, kLedDriverRefCurrent,
                kLedConfig)) {

}

/**
 * @brief Tear down HMI driver resources
 *
 * Stops all of our background timers we created.
 */
HmiDriver::~HmiDriver() {

}

/**
 * @brief Handles a front panel IRQ
 *
 * Read the IO expander to determine which buttons changed state.
 */
void HmiDriver::handleIrq() {
    // TODO: implement
}

/**
 * @brief Update indicator state
 *
 * Updates the state of which LEDs should be illuminated on the front panel, then writes the
 * appropriate registers in the LED driver.
 *
 * TODO: handle gradation/blinking as needed
 */
int HmiDriver::setIndicatorState(const Indicator state) {
    // TODO: implement
    return -1;
}

