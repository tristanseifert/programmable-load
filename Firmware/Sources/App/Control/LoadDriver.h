#ifndef APP_CONTROL_LOADDRIVER_H
#define APP_CONTROL_LOADDRIVER_H

#include <stddef.h>
#include <stdint.h>

namespace Drivers {
class I2CBus;

namespace I2CDevice {
class AT24CS32;
}
}

namespace App::Control {
/**
 * @brief Interface for a load driver
 *
 * This is an abstract base class that defines the methods used by the control loop task to
 * communicate with the actual analog (load) board.
 *
 * Primarily, this is geared towards analog boards that don't have a lot of smarts locally, and
 * need a lot of hand-holding from the actual control loop running in the control task.
 */
class LoadDriver {
    public:
        /**
         * @brief Initialize driver
         *
         * Set up all hardware associated with the board, and resets it.ensuring that the load is
         * not drawing current.
         */
        LoadDriver(Drivers::I2CBus *bus, Drivers::I2CDevice::AT24CS32 &idprom) : bus(bus) {};

        /**
         * @brief Shut down driver
         *
         * Release all resources and again disable the load.
         */
        virtual ~LoadDriver() = default;



        /**
         * @brief Interrupt handler
         *
         * Invoked when the driver board asserts its interrupt line.
         *
         * @remark This is invoked from the context of the control loop task, BEFORE it runs the
         *         next invocation of the loop.
         *
         * @remark The default implementation of this handler does nothing.
         */
        virtual void handleIrq() {};



        /**
         * @brief Set the state of the load
         *
         * @param isEnabled When set, the load is enabled and consumes current.
         *
         * @return 0 on success or negative error code
         */
        virtual int setEnabled(const bool isEnabled) = 0;


        /**
         * @brief Read input voltage
         *
         * Read the voltage at the active voltage sense input.
         *
         * @param outVoltage Variable to receive current voltage (in mV)
         *
         * @return 0 on success, error code otherwise
         */
        virtual int readInputVoltage(uint32_t &outVoltage) = 0;

        /**
         * @brief Set the source of voltage sense
         *
         * @param isExternal Whether the external voltage sense input should be used
         *
         * @return 0 on success or negative error code
         */
        virtual int setExternalVSense(const bool isExternal) = 0;

    protected:
        /// The I2C bus to which the load board is connected
        Drivers::I2CBus *bus;
};
}

#endif
