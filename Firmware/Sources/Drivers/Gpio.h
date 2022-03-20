#ifndef DRIVERS_GPIO_H
#define DRIVERS_GPIO_H

#include <stdint.h>

namespace Drivers {
/**
 * @brief PORT / GPIO driver
 *
 * Provides methods to configure IO pins, as well as interface with the digital input/output pins.
 */
class Gpio {
    public:
        /**
         * @brief GPIO port
         *
         * The device has multiple "banks" of IO ports, each identified by a single letter. Inside
         * each port are up to 32 pins, numbered 0-31.
         *
         * @remark Port D and E are only supported in larger devices. We can add them easily when
         *         needed down the road.
         */
        enum class Port: uint8_t {
            PortA,
            PortB,
            PortC,
        };

        /**
         * @brief IO pin mode
         *
         * Defines the direction and/or mode of the IO pin.
         */
        enum class Mode: uint8_t {
            /// Pin is disabled (unused)
            Off,
            /// Configure pin as a digital input
            DigitalIn,
            /// Configure pin as a digital output
            DigitalOut,
            /// Use the pin for analog functions
            Analog,
            /**
             * @brief Peripheral multiplexer mode
             *
             * The direction and output drive is controlled by the peripheral associated by the
             * port mux function specified in the `function` field of PinMode.
             */
            Peripheral,
        };

        /// Pull up/down resistor configuration
        enum class Pull: uint8_t {
            /// No pull up/down resistors
            None,
            /// Enable a pull-up resistor (towards VCC)
            Up,
            /// Enable a pull-down resistor (towards GND)
            Down,
        };

        /**
         * @brief Pin mode definition
         *
         * Encapsulates the configuration for a particular pin. This includes whether the pin is an
         * input or output, pull up/down resistors, or whether the pin is in use for an alternate
         * function.
         */
        struct PinConfig {
            /**
             * @brief Pin mode
             *
             * Specifies the way this pin is used, including the peripherals that may have access
             * to it or how it is used.
             */
            Mode mode{Mode::Off};

            /**
             * @brief Pull resistor configuration
             *
             * When the pin is disabled, or configured as digital IO, configurable pull-up or pull-
             * down resistors may be enabled.
             */
            Pull pull{Pull::None};

            /**
             * @brief Peripheral function
             *
             * @remark Only relevant when the pin mode is Mode::PortMux.
             */
            uint8_t function: 4{0};

            /**
             * @brief High drive strength
             *
             * When set (1) the pin uses a higher drive strength value.
             */
            uint8_t driveStrength: 1{0};

            /**
             * @brief Initial output state
             *
             * When configured as a digital output, this bit is written to the output latch before
             * the output driver is enabled, so the pin starts at a predictable level.
             */
            uint8_t initialOutput: 1{0};

            /**
             * @brief Enable pin mux
             *
             * If the pin is configured as a digital input or output, set this bit to enable the
             * pin mux to the specified function. This can be used to, for example, route an input
             * to the external interrupt controller.
             */
            uint8_t pinMuxEnable: 1{0};
        };

    public:
        Gpio() = delete;

        static void ConfigurePin(const Port port, const uint8_t pin, const PinConfig &config);
        static void SetOutputState(const Port port, const uint8_t pin, const bool state);
        static bool GetInputState(const Port port, const uint8_t pin);
};
}

#endif
