#ifndef APP_PINBALL_FROMTIO_HMIDRIVER_H
#define APP_PINBALL_FROMTIO_HMIDRIVER_H

#include <stddef.h>
#include <stdint.h>

#include "../FrontIoDriver.h"

#include "Drivers/I2CDevice/PCA9955B.h"
#include "Drivers/I2CDevice/XRA1203.h"

#include "Rtos/Rtos.h"
#include "Util/Uuid.h"

#include <etl/array.h>

namespace App::Pinball {
/**
 * @brief Driver for "Programmable load HMI"
 *
 * This implements the driver for the front panel IO board in the `Hardware` directory, featuring
 * an XRA1203 IO expander for buttons, and a PCA9955B LED driver. There is also a rotary encoder,
 * but the signals are passed to the processor board to handle.
 *
 * TODO: read LED currents from IDPROM
 */
class HmiDriver: public FrontIoDriver {
    public:
        /// Driver UUID bytes
        constexpr static const etl::array<uint8_t, Util::Uuid::kByteSize> kUuidBytes{{
            0xde, 0xf5, 0x21, 0x2a, 0x92, 0x76, 0x47, 0xd7, 0x93, 0xb4, 0x5e, 0x25, 0x52, 0x6a, 0x8c, 0x95
        }};

        /**
         * @brief Driver UUID
         *
         * Use this UUID in the inventory ROM on the front panel to match the driver.
         */
        static const Util::Uuid kDriverId;

    public:
        HmiDriver(Drivers::I2CBus *bus, Drivers::I2CDevice::AT24CS32 &idprom);
        ~HmiDriver() override;

        void handleIrq() override;
        int setIndicatorState(const Indicator state) override;

    private:
        /// Bus address of the LED driver
        constexpr static const uint8_t kLedDriverAddress{0b0000011};
        /**
         * @brief LED driver reference current (µA)
         *
         * This is the reference current set by the RExt resistor on the LED driver, and is scaled
         * to each of the LED outputs. This board has a 2kΩ resistor, which sets a maximum current
         * value of roughly 28mA.
         */
        constexpr static const uint16_t kLedDriverRefCurrent{28687};
        /// LED driver configuration
        constexpr static const etl::array<const Drivers::I2CDevice::PCA9955B::LedConfig, 16> 
            kLedConfig{{
            // Status RGB green channel
            {
                .enabled = 1,
                .fullCurrent = 5000,
            },
            // Status RGB red channel
            {
                .enabled = 1,
                .fullCurrent = 5000,
            },

            // Mode selector, bonus mode
            {
                .enabled = 1,
                .fullCurrent = 10000,
            },
            // Mode selector, constant wattage
            {
                .enabled = 1,
                .fullCurrent = 10000,
            },
            // Mode selector, constant voltage
            {
                .enabled = 1,
                .fullCurrent = 10000,
            },
            // Mode selector, constant current
            {
                .enabled = 1,
                .fullCurrent = 10000,
            },

            // input enable (green)
            {
                .enabled = 1,
                .fullCurrent = 10000,
            },
            // input enable (red)
            {
                .enabled = 1,
                .fullCurrent = 10000,
            },

            // menu
            {
                .enabled = 1,
                .fullCurrent = 15000,
            },
            // status RGB led (blue)
            {
                .enabled = 1,
                .fullCurrent = 5000,
            },

            // unused x2
            { .enabled = 0 },
            { .enabled = 0 },

            // limiting (amber)
            {
                .enabled = 1,
                .fullCurrent = 2500,
            },
            // overheat (red)
            {
                .enabled = 1,
                .fullCurrent = 2500,
            },
            // overcurrent (red)
            {
                .enabled = 1,
                .fullCurrent = 2500,
            },
            // error (red)
            {
                .enabled = 1,
                .fullCurrent = 2500,
            },
        }};

        /**
         * @brief Mapping of LEDs to channels on the LED driver
         */
        enum class LedChannel: uint8_t {
            /// Status indicator, green channel
            StatusG                     = 0,
            /// Status indicator, red channel
            StatusR                     = 1,
            /// Status indicator, blue channel
            StatusB                     = 9,
            /// Mode selector, bonus mode
            ModeExt                     = 2,
            /// Mode selector, constant wattage
            ModeCW                      = 3,
            /// Mode selector, constant voltage
            ModeCV                      = 4,
            /// Mode selector, constant current
            ModeCC                      = 5,
            /// Input enable, green channel
            InputEnableG                = 6,
            /// Input enable, red channel
            InputEnableR                = 7,
            /// Menu button
            Menu                        = 8,
            /// Limiting indicator (amber)
            LimitingOn                  = 12,
            /// Overheat (red)
            Overheat                    = 13,
            /// Overcurrent (red)
            Overcurrent                 = 14,
            /// General error (red)
            Error                       = 15,
        };

        /// Bus address of the IO expander
        constexpr static const uint8_t kExpanderAddress{0b0100000};
        /// IO expander pin configuration
        constexpr static const etl::array<Drivers::I2CDevice::XRA1203::PinConfig, 16> kPinConfigs{{
            // Menu button
            {
                .input = true,
                .pullUp = true,
                .invertInput = true,
                .irq = true,
                .irqRising = true,
                .irqFalling = true,
                .irqFilter = true,
            },
            // Encoder push switch
            {
                .input = true,
                .pullUp = true,
                .invertInput = true,
                .irq = true,
                .irqRising = true,
                .irqFalling = true,
                .irqFilter = true,
            },
            // unused IOs x6
            Drivers::I2CDevice::XRA1203::kPinConfigUnused,
            Drivers::I2CDevice::XRA1203::kPinConfigUnused,
            Drivers::I2CDevice::XRA1203::kPinConfigUnused,
            Drivers::I2CDevice::XRA1203::kPinConfigUnused,
            Drivers::I2CDevice::XRA1203::kPinConfigUnused,
            Drivers::I2CDevice::XRA1203::kPinConfigUnused,

            // LED driver output enable (we don't use this)
            {
                .input = false,
                .initialOutput = false,
            },
            // unused IOs x2
            Drivers::I2CDevice::XRA1203::kPinConfigUnused,
            Drivers::I2CDevice::XRA1203::kPinConfigUnused,
            // input enable
            {
                .input = true,
                .pullUp = true,
                .invertInput = true,
                .irq = true,
                .irqFalling = true,
                .irqFilter = true,
            },
            // constant current mode
            {
                .input = true,
                .pullUp = true,
                .invertInput = true,
                .irq = true,
                .irqFalling = true,
                .irqFilter = true,
            },
            // constant voltage mode
            {
                .input = true,
                .pullUp = true,
                .invertInput = true,
                .irq = true,
                .irqFalling = true,
                .irqFilter = true,
            },
            // constant wattage mode
            {
                .input = true,
                .pullUp = true,
                .invertInput = true,
                .irq = true,
                .irqFalling = true,
                .irqFilter = true,
            },
            // bonus mode
            {
                .input = true,
                .pullUp = true,
                .invertInput = true,
                .irq = true,
                .irqFalling = true,
                .irqFilter = true,
            },
        }};
        /// Bitmask of IO lines that have buttons connected
        constexpr static const uint16_t kIoButtonMask{0xF803};
        /// Input line for the menu button
        constexpr static const uint16_t kIoButtonMenu{1 << 0};
        /// Input line for the select (encoder) button
        constexpr static const uint16_t kIoButtonSelect{1 << 1};
        /// Input line for the input enable button
        constexpr static const uint16_t kIoButtonInputEnable{1 << 11};
        /// Input line for the constant current mode button
        constexpr static const uint16_t kIoButtonModeCC{1 << 12};
        /// Input line for the constant voltage mode button
        constexpr static const uint16_t kIoButtonModeCV{1 << 13};
        /// Input line for the constant wattage mode button
        constexpr static const uint16_t kIoButtonModeCW{1 << 14};
        /// Input line for the bonus mode button
        constexpr static const uint16_t kIoButtonModeExt{1 << 15};

    private:
        /// LED driver
        Drivers::I2CDevice::PCA9955B ledDriver;
        /// Button IO expander
        Drivers::I2CDevice::XRA1203 ioExpander;

        /// Current state of the buttons
        Button buttonState{0};
        /**
         * @brief Current indicator state
         *
         * This is initialized to all 1's so that on the first request to set the indicators, we'll
         * update every indicator on the board.
         */
        Indicator indicatorState{UINTPTR_MAX};

        /**
         * @brief IO state poll timer
         *
         * We'll periodically poll the front panel for input changes by injecting a fake front
         * panel interrupt. This is used in case we miss an interrupt (due to noise on the line,
         * or a really bouncy switch) to ensure interrupts keep coming in.
         *
         * The timer is restarted any time we receive an interrupt. So, as long as we keep getting
         * events more frequently than our interval, it won't do anything.
         */
        TimerHandle_t ioPollTimer;
        StaticTimer_t ioPollTimerStorage;

        /**
         * @brief Interval for IO state poll timer (msec)
         *
         * Select this as high as possible to reduce the performance impact of the timer, but not
         * so high that it would cause too much lag when pressing buttons.
         */
        constexpr static const size_t kIoPollTimerInterval{500};
};
}

#endif
