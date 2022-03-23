#ifndef APP_MAIN_HARDWARE_H
#define APP_MAIN_HARDWARE_H

#include <stddef.h>
#include <stdint.h>

#include <etl/span.h>

#include "Drivers/Gpio.h"

namespace Drivers {
class I2C;
class I2CBus;

namespace I2CDevice {
class PCA9543A;
}
}

namespace App::Main {
class Hw {
    /// IO bus - I²C SCL
    constexpr static const Drivers::Gpio::Pin kIoBusScl{
        Drivers::Gpio::Port::PortA, 9
    };
    /// IO bus - I²C SDA
    constexpr static const Drivers::Gpio::Pin kIoBusSda{
        Drivers::Gpio::Port::PortA, 8
    };
    /// IO bus - I²C IRQ
    constexpr static const Drivers::Gpio::Pin kIoBusIrq{
        Drivers::Gpio::Port::PortA, 10
    };
    /// IO bus - mux reset
    constexpr static const Drivers::Gpio::Pin kIoBusReset{
        Drivers::Gpio::Port::PortA, 6
    };

    public:
        static Drivers::I2C *InitIoBus();
        static void InitIoBusMux(etl::span<Drivers::I2CBus *, 2> outBusses);

        /**
         * @brief Get the IO bus mux
         */
        static auto GetIoMux() {
            return gIoMux;
        }

        static int QueryIoIrq(bool &front, bool &rear);

    private:
        /**
         * @brief Set the state of the /I2C_RESET line
         *
         * This is connected to the multiplexer's reset line, and allows resetting its internal
         * state machine on powerup if the bus got wedged in a weird state. It does not reset
         * devices beyond the multiplexer, on either of the secondary busses.
         *
         * @param asserted When set, the reset line is asserted (low)
         */
        inline static void SetIoBusReset(const bool asserted) {
            Drivers::Gpio::SetOutputState(kIoBusReset, !asserted);
        }

    private:
        /**
         * @brief I²C address of bus switch
         *
         * The 7-bit bus address of the bus switch/multiplexer.
         */
        constexpr static const uint8_t kIoMuxAddress{0b111'0000};

        /**
         * @brief IO bus I²C master
         *
         * Instance of the SERCOM that's driving the I²C bus connected to the front and rear IO
         * connectors. This bus is also shared with a few on-board peripherals.
         */
        static Drivers::I2C *gIoBus;

        /**
         * @brief IO bus mux
         *
         * Switch to split the IO bus into separate front and rear IO busses
         */
        static Drivers::I2CDevice::PCA9543A *gIoMux;
};
}

#endif
