#ifndef APP_PINBALL_HARDWARE_H
#define APP_PINBALL_HARDWARE_H

#include <stdint.h>

#include <etl/span.h>

#include "Drivers/Gpio.h"

namespace Drivers {
class I2CBus;
class Spi;
class TimerCounter;
}

namespace App::Pinball {
/**
 * @brief Illumination mode for the power button
 */
enum class PowerLightMode {
    /// Power button is not illuminated
    Off,
    /// Illuminate in primary color
    Primary,
    /// Enable secondary color
    Secondary,
};

/**
 * @brief Hardware owned by the pinball task
 *
 * This consists of the SPI for the display, the I2C bus for the front panel (which is hidden
 * behind a multiplexer) and some general IO.
 */
class Hw {
    friend class Task;

    /**
     * @brief Front panel reset
     *
     * Active low signal to reset all peripherals on the front panel, including the display.
     */
    constexpr static const Drivers::Gpio::Pin kFrontIoReset{
        Drivers::Gpio::Port::PortA, 5
    };

    /// Display SPI - SCK
    constexpr static const Drivers::Gpio::Pin kDisplaySck{
        Drivers::Gpio::Port::PortB, 13
    };
    /// Display SPI - MISO
    constexpr static const Drivers::Gpio::Pin kDisplayMiso{
        Drivers::Gpio::Port::PortB, 12
    };
    /// Display SPI - MOSI
    constexpr static const Drivers::Gpio::Pin kDisplayMosi{
        Drivers::Gpio::Port::PortB, 15
    };
    /// Display SPI - Chip select (/CS)
    constexpr static const Drivers::Gpio::Pin kDisplayCs{
        Drivers::Gpio::Port::PortB, 14
    };
    /// Display - command/data strobe
    constexpr static const Drivers::Gpio::Pin kDisplayCmdData{
        Drivers::Gpio::Port::PortA, 4
    };

    /// Power button - switch input
    constexpr static const Drivers::Gpio::Pin kPowerSwitch{
        Drivers::Gpio::Port::PortB, 31
    };
    /**
     * @brief Power LED
     *
     * Line to control the bicolor LED inside the power switch; 0 activates the primary color, 1
     * activates the secondary. Put the pin in hi-Z/input mode to extinguish the LED.
     */
    constexpr static const Drivers::Gpio::Pin kPowerIndicator{
        Drivers::Gpio::Port::PortB, 27
    };

    /// Rotary encoder, A line
    constexpr static const Drivers::Gpio::Pin kEncoderA{
        Drivers::Gpio::Port::PortB, 7
    };
    /// Rotary encoder, B line
    constexpr static const Drivers::Gpio::Pin kEncoderB{
        Drivers::Gpio::Port::PortB, 8
    };

    /// Beeper output
    constexpr static const Drivers::Gpio::Pin kBeeper{
        Drivers::Gpio::Port::PortB, 10
    };

    public:
        static void Init(const etl::span<Drivers::I2CBus *, 2> &busses);

    private:
        static void InitDisplaySpi();
        static void InitPowerButton();
        static void InitEncoder();
        static void InitBeeper();
        static void InitMisc();

        static void ResetFrontPanel();

        static void SetDisplayDataCommandFlag(const bool isData);
        static void SetPowerLight(const PowerLightMode mode);
        static uint8_t ReadEncoder();

        /// Display SPI bus
        static Drivers::Spi *gDisplaySpi;
        /// Timer used to drive beeper in waveform mode
        static Drivers::TimerCounter *gBeeperTc;

        /// Front panel bus
        static Drivers::I2CBus *gFrontI2C;
        /// rear IO bus
        static Drivers::I2CBus *gRearI2C;
};
}

#endif
