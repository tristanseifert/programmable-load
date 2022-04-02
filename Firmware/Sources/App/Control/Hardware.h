#ifndef APP_CONTROL_HARDWARE_H
#define APP_CONTROL_HARDWARE_H

#include "Drivers/Gpio.h"

namespace Drivers {
class I2C;
}

/// Control loop
namespace App::Control {
/**
 * @brief Control loop hardware
 *
 * This is responsible for initializing all hardware used by the actual load control; that is, the
 * control I²C bus, and a few related GPIOs.
 */
class Hw {
    friend class Task;

    /**
     * @brief External trigger input
     *
     * An active low signal input from the driver board, used for triggering in some of the
     * controller's modes.
     */
    constexpr static const Drivers::Gpio::Pin kExternalTrigger{
        Drivers::Gpio::Port::PortB, 11
    };

    /**
     * @brief Driver reset output
     *
     * Active low reset strobe for the driver. When asserted, it should disable all outputs.
     */
    constexpr static const Drivers::Gpio::Pin kDriverReset{
        Drivers::Gpio::Port::PortB, 6
    };

    /**
     * @brief Driver interrupt input
     *
     * Active low interrupt input, asserted when the driver board requires service.
     */
    constexpr static const Drivers::Gpio::Pin kDriverIrq{
        Drivers::Gpio::Port::PortB, 9
    };

    /**
     * @brief Driver I²C clock
     *
     * SERCOM3, PAD1
     */
    constexpr static const Drivers::Gpio::Pin kDriverScl{
        Drivers::Gpio::Port::PortA, 23
    };

    /**
     * @brief Driver I²C data
     *
     * SERCOM3, PAD0
     */
    constexpr static const Drivers::Gpio::Pin kDriverSda{
        Drivers::Gpio::Port::PortA, 22
    };


    public:
        static void Init();

        static void PulseReset();
        static void SetResetState(const bool asserted);

    private:
        /**
         * @brief Driver control bus
         *
         * Dedicated I²C bus used for communicating with the load driver board.
         */
        static Drivers::I2C *gBus;
};
}

#endif
