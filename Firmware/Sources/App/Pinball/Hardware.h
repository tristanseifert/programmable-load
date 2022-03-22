#ifndef APP_PINBALL_HARDWARE_H
#define APP_PINBALL_HARDWARE_H

#include <stdint.h>

#include <etl/span.h>

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