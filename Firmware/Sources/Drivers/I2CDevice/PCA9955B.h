#ifndef DRIVERS_I2CDEVICE_PCA9955B_H
#define DRIVERS_I2CDEVICE_PCA9955B_H

#include <stddef.h>
#include <stdint.h>

#include <etl/array.h>
#include <etl/span.h>

namespace Drivers {
class I2CBus;
}

namespace Drivers::I2CDevice {
/**
 * @brief 16-channel constant current LED driver
 *
 * A basic driver for the PCA9955B LED driver. It features 16 individually controllable output
 * channels, each of which may sink up to 57mA. Each LED has an individually programmable current
 * gain, as well as PWM for brightness.
 *
 * On top of this, the chip provides hardware controlled blinking/gradation support.
 */
class PCA9955B {
    public:
        /// Total number of LED output channels
        constexpr static const size_t kNumChannels{16};

        /**
         * @brief Error codes emitted by the device
         */
        enum Errors: int {
            /**
             * @brief Invalid channel number
             */
            InvalidChannel              = -5400,
        };

        /**
         * @brief Configuration for a single LED channel
         */
        struct LedConfig {
            /**
             * @brief Is the channel enabled?
             *
             * Clear this flag to completely disable the output driver for this channel.
             */
            uint8_t enabled:1{1};

            /**
             * @brief Gradation channel
             *
             * Which one of the four gradation groups this LED channel belongs to. When that group
             * is enabled, this LED's brightness will be controlled by that gradation channel.
             */
            uint8_t gradationGroup:2{0};

            /**
             * @brief Full brightness current
             *
             * Specify the current through this LED for full brightness. It is specified in
             * microamps (µA)
             */
            uint16_t fullCurrent{0};
        };

    public:
        PCA9955B(Drivers::I2CBus *bus, const uint8_t busAddress, const uint16_t refCurrent,
                etl::span<const LedConfig, kNumChannels> config);
        ~PCA9955B();

        int setBrightness(const uint8_t channel, const float level);

    private:
        enum Regs: uint8_t {
            Mode1                       = 0x00,
            Mode2                       = 0x01,
            /**
             * @brief LED output state 0
             *
             * Output state configuration for channels 0-3
             */
            LEDOUT0                     = 0x02,
            /**
             * @brief LED output state 1
             *
             * Output state configuration for channels 0-3
             */
            LEDOUT1                     = 0x03,
            /**
             * @brief LED output state 2
             *
             * Output state configuration for channels 0-3
             */
            LEDOUT2                     = 0x04,
            /**
             * @brief LED output state 3
             *
             * Output state configuration for channels 0-3
             */
            LEDOUT3                     = 0x05,
            /**
             * @brief Channel 0 brightness
             *
             * Controls the PWM duty cycle of channel 0. Channels 1-15 follow sequentially after
             * this register.
             */
            PWM0                        = 0x08,
            /**
             * @brief Channel 0 current
             *
             * The output current/gain, proportional to the main current reference specified at
             * creation time. This, in turn, is set by the Rext resistor.
             */
            IREF0                       = 0x18,
            /**
             * @brief Gradation group select 0
             *
             * Gradation group selection for LED channels 0-3
             */
            GradationGroup0             = 0x3A,
            /**
             * @brief Gradation group select 1
             *
             * Gradation group selection for LED channels 4-7
             */
            GradationGroup1             = 0x3B,
            /**
             * @brief Gradation group select 0
             *
             * Gradation group selection for LED channels 8-11
             */
            GradationGroup2             = 0x3C,
            /**
             * @brief Gradation group select 0
             *
             * Gradation group selection for LED channels 12-15
             */
            GradationGroup3             = 0x3D,
            /**
             * @brief Brightness control for all outputs
             *
             * Any writes to this register are mirrored to all of the 16 channels' brightness
             * PWM outputs.
             */
            PWMALL                      = 0x44,
        };

    private:
        /// I²C bus holding the LED driver
        Drivers::I2CBus *bus;
        /// LED output Reference current, in µA
        uint16_t refCurrent;
        /// Address of the device on the bus
        uint8_t busAddress;
};
}

#endif
