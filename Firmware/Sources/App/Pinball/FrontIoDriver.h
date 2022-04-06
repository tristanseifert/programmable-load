#ifndef APP_PINBALL_FRONTIODRIVER_H
#define APP_PINBALL_FRONTIODRIVER_H

#include <stddef.h>
#include <stdint.h>

namespace Drivers {
class I2CBus;

namespace I2CDevice {
class AT24CS32;
}
}

namespace App::Pinball {
/**
 * @brief Interface for a front panel IO driver
 *
 * This is an abstract base class that defines the methods used by the UI task to interact with a
 * front panel board. 
 */
class FrontIoDriver {
    public:
        /**
         * @brief Various indicators available on the front panel
         *
         * These indicator constants can map to an indicator (LED) on the front panel, or perhaps
         * set the color of a multicolor indicator.
         */
        enum Indicator: uintptr_t {
            /**
             * @brief Overheat error
             *
             * Indicates the system is overheating.
             */
            Overheat                    = (1 << 0),

            /**
             * @brief Current exceeded
             *
             * The input current exceeded the maximum allowable current.
             */
            Overcurrent                 = (1 << 1),

            /**
             * @brief Generic error
             *
             * Some sort of error has occurred during the operation.
             */
            GeneralError                = (1 << 2),

            /**
             * @brief Input active
             *
             * Current sinking input is enabled
             */
            InputEnabled                = (1 << 3),

            /**
             * @brief Menu button
             *
             * Used by the UI layer to indicate that the menu is activated
             */
            Menu                        = (1 << 4),

            /**
             * @brief Constant current mode enabled
             */
            ModeCC                      = (1 << 5),
            /**
             * @brief Constant voltage mode enabled
             */
            ModeCV                      = (1 << 6),
            /**
             * @brief Constant wattage mode enabled
             */
            ModeCW                      = (1 << 7),
            /**
             * @brief Bonus mode enabled
             */
            ModeExt                     = (1 << 8),
        };

    public:
        /**
         * @brief Initialize driver
         *
         * Set up all hardware on the driver. It's attached to the specified bus. The driver may
         * read from the provided IDPROM to find more data.
         */
        FrontIoDriver(Drivers::I2CBus *bus, Drivers::I2CDevice::AT24CS32 &idprom) : bus(bus) {};

        /**
         * @brief Shut down driver
         *
         * Release all resources associated with the driver, and reset any indicators.
         */
        virtual ~FrontIoDriver() = default;



        /**
         * @brief Interrupt handler
         *
         * Invoked when the front IO interrupt line is asserted.
         *
         * @remark This is called from a regular task context, not the ISR context.
         *
         * @remark The default implementation of this handler does nothing.
         */
        virtual void handleIrq() {};

        /**
         * @brief Set indicator state
         *
         * Updates the state of all indicators on the IO board. This should be a bitwise OR of the
         * indicator values above.
         *
         * @param state Bitwise OR of indicator values to set
         */
        virtual int setIndicatorState(const Indicator state) = 0;

    protected:
        /// The I2C bus to which the front IO board is connected
        Drivers::I2CBus *bus;
};
}

#endif
