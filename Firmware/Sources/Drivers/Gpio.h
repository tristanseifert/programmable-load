#ifndef DRIVERS_GPIO_H
#define DRIVERS_GPIO_H

#include <stdint.h>

#include <etl/utility.h>

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
         * each port are up to 16 pins, numbered 0-31.
         */
        enum class Port: uint8_t {
            PortA,
            PortB,
            PortC,
            PortD,
            PortE,
            PortF,
            PortG,
            PortH,
            PortI,
            // Port Z is accessible only from the A7 core
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
             * @brief Peripheral multiplexer mode (alternate function)
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
         * @brief Pin location
         *
         * A combination of GPIO port and pin that can be used to uniquely identify a GPIO pin.
         */
        using Pin = etl::pair<Port, uint8_t>;

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
             * @brief Whether the output operates in open drain mode
             *
             * When set, the output operates in open drain mode: that is, the output will only be
             * pulled low. This bit is ignored if the pin is not configured as an output.
             */
            uint8_t isOpenDrain: 1{0};

            /**
             * @brief Peripheral function
             *
             * @remark Only relevant when the pin mode is Mode::Peripheral.
             */
            uint8_t function: 4{0};

            /**
             * @brief Pin IO speed
             *
             * Sets the expected IO output speed, from low (0) to highest (3). This affects the
             * drive strength and other properties.
             */
            uint8_t speed: 2{1};

            /**
             * @brief Initial output state
             *
             * When configured as a digital output, this bit is written to the output latch before
             * the output driver is enabled, so the pin starts at a predictable level.
             */
            uint8_t initialOutput: 1{0};
        };

    public:
        Gpio() = delete;

        static void ConfigurePin(const Pin pin, const PinConfig &config);

        static void SetOutputState(const Pin pin, const bool state);

        static bool GetInputState(const Pin pin);

    private:
        static void AcquireLock();
        static void UnlockGpio();

        /// ID of the semaphore slot used for GPIOs; fixed by linux code
        constexpr static const size_t kSemaphoreId{0};
        /// Maximum number of times to try acquiring the lock before giving up
        constexpr static const size_t kLockRetries{5000};
};
}

#endif
