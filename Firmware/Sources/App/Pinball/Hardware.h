#ifndef APP_PINBALL_HARDWARE_H
#define APP_PINBALL_HARDWARE_H

#include <stdint.h>

#include <etl/span.h>

#include "Drivers/Gpio.h"

extern "C" {
void EIC_8_Handler();
void EIC_7_Handler();
}

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
    friend void ::EIC_7_Handler();
    friend void ::EIC_8_Handler();

    friend class Task;
    friend class Display;

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

    /// Constants used by the encoder state machine
    enum EncoderState: uint8_t {
        Initial                         = 0,
        CwFinal                         = 1,
        CwBegin                         = 2,
        CwNext                          = 3,
        CcwBegin                        = 4,
        CcwFinal                        = 5,
        CcwNext                         = 6,
    };
    /// Encoder rotation direction
    enum EncoderDirection: uint8_t {
        DirectionNone                   = 0,
        /// One step clockwise
        DirectionCW                     = (1 << 4),
        /// One step counterclockwise
        DirectionCCW                    = (1 << 5),
        /// Bitmask for direction values
        DirectionMask                   = (DirectionCW | DirectionCCW),
    };
    /**
     * @brief State table for encoder state machine
     *
     * This is a 2 dimensional array, where the first index is the current state, and the second
     * index is the current state of the IO lines.
     */
    static const uint8_t kEncoderStateTable[7][4];

    public:
        static void Init(const etl::span<Drivers::I2CBus *, 2> &busses);

        static void ResetFrontPanel();

        /**
         * @brief Set the state of the display chip select line
         */
        inline static void SetDisplaySelect(const bool isSelected) {
            Drivers::Gpio::SetOutputState(kDisplayCs, !isSelected);
        }

        /**
         * @brief Set the state of the display `D/C` line
         *
         * @param isData Whether the next byte contains data or commands
         */
        inline static void SetDisplayDataCommandFlag(const bool isData) {
            Drivers::Gpio::SetOutputState(kDisplayCmdData, isData);
        }

        /**
         * @brief Read the state of the two encoder pins.
         *
         * @return Encoder state; bit 0 is the A line, bit 1 the B line.
         */
        static inline uint8_t ReadEncoder() {
            uint8_t temp{0};
            if(Drivers::Gpio::GetInputState(kEncoderA)) {
                temp |= (1 << 0);
            }
            if(Drivers::Gpio::GetInputState(kEncoderB)) {
                temp |= (1 << 1);
            }
            return temp;
        }
        /**
         * @brief Read, then reset the encoder delta
         *
         * Atomically reads out the encoder delta (steps since last read) then resets it to
         * zero.
         */
        static inline int ReadEncoderDelta() {
            int zero{0}, value;
            __atomic_exchange(&gEncoderDelta, &zero, &value, __ATOMIC_RELAXED);
            return value;
        }

        static void SetPowerLight(const PowerLightMode mode);

    private:
        static void InitDisplaySpi();
        static void InitPowerButton();
        static void InitEncoder();
        static void InitBeeper();
        static void InitMisc();

        static void AdvanceEncoderState(long *);

        /// Display SPI bus
        static Drivers::Spi *gDisplaySpi;
        /// Timer used to drive beeper in waveform mode
        static Drivers::TimerCounter *gBeeperTc;

        /// Front panel bus
        static Drivers::I2CBus *gFrontI2C;
        /// rear IO bus
        static Drivers::I2CBus *gRearI2C;

        /// current state of encoder state machine
        static uint8_t gEncoderCurrentState;
        /**
         * @brief Current encoder sum
         *
         * This is an integer value, initialized to 0 at startup, and any time the pinball task
         * has read (and then reset) this value. For each step of the encoder, 1 or -1 is added to
         * it, depending on the rotation direction.
         */
        static int gEncoderDelta;
};
}

#endif
