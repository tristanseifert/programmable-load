#ifndef APP_MAIN_HARDWARE_H
#define APP_MAIN_HARDWARE_H

#include <stddef.h>
#include <stdint.h>

#include <etl/span.h>

namespace Drivers {
class I2C;
class I2CBus;

namespace I2CDevice {
class PCA9543A;
}
}

namespace App::Main {
class Hw {
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
        static void SetIoBusReset(const bool asserted);

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
