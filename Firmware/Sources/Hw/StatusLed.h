#ifndef HW_STATUSLED_H
#define HW_STATUSLED_H

#include "Drivers/Gpio.h"

namespace Hw {
/**
 * @brief On-board RGB status indicator
 *
 * Interface driver for the on-board status LED. It's connected directly to some GPIOs and can be
 * set to any one of 7 colors. (In the future, we may support more colors by means of PWM.)
 */
class StatusLed {
    public:
        /// Possible color values for the status LED
        enum class Color: uint8_t {
            Off                         = 0b000,
            Blue                        = 0b001,
            Green                       = 0b010,
            Cyan                        = 0b011,
            Red                         = 0b100,
            Magenta                     = 0b101,
            Yellow                      = 0b110,
            White                       = 0b111,
        };

        /**
         * @brief Initialize the status LED
         *
         * This sets up the GPIO pins the LED is connected to. The LED will be off.
         */
        static void Init() {
            const Drivers::Gpio::PinConfig cfg{
                .mode = Drivers::Gpio::Mode::DigitalOut,
                .initialOutput = 1,
            };

            Drivers::Gpio::ConfigurePin(kPinRed, cfg);
            Drivers::Gpio::ConfigurePin(kPinGreen, cfg);
            Drivers::Gpio::ConfigurePin(kPinBlue, cfg);
        }

        /**
         * @brief Set the color of the status LED
         *
         * This will update the GPIO state accordingly.
         */
        static void Set(const Color col) {
            const uint8_t bits = static_cast<uint8_t>(col);

            Drivers::Gpio::SetOutputState(kPinRed, !(bits & 0b100));
            Drivers::Gpio::SetOutputState(kPinGreen, !(bits & 0b010));
            Drivers::Gpio::SetOutputState(kPinBlue, !(bits & 0b001));
        }

    private:
        /// Red pin of the status LED (active low)
        constexpr static const Drivers::Gpio::Pin kPinRed{Drivers::Gpio::Port::PortG, 5};
        /// Green pin of the status LED (active low)
        constexpr static const Drivers::Gpio::Pin kPinGreen{Drivers::Gpio::Port::PortD, 13};
        /// Blue in of the status LED (active low)
        constexpr static const Drivers::Gpio::Pin kPinBlue{Drivers::Gpio::Port::PortF, 8};
};
};

#endif
