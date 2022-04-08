#ifndef APP_PINBALL_FRONTIO_DISPLAY_H
#define APP_PINBALL_FRONTIO_DISPLAY_H

#include <stddef.h>
#include <stdint.h>

#include <etl/array.h>
#include <etl/span.h>

namespace Drivers {
class Spi;
}

namespace App::Pinball {
/**
 * @brief Front panel OLED display driver
 *
 * Implements a basic driver on top of the display SPI to communicate with a 256x64, 4bpp greyscale
 * OLED display. It supports the SSD1322 controller. Currently, we copy the entire framebuffer when
 * the display changes.
 *
 * Specifically, this driver is optimized to work with ER-OLEDM032 displays, but should work with
 * any other display based on the same controller.
 *
 * @remark The display driver is not thread safe; you should only access it from one thread at a
 *         time.
 */
class Display {
    private:
        /**
         * @brief Commands for the SSD1322 display controller
         *
         * See each command's remark to see if any additional data follows the command.
         */
        enum class Command: uint8_t {
            /**
             * @brief Set column address range
             *
             * Define the columns (in the framebuffer) that will be accessed by an upcoming
             * framebuffer write. The payload is two bytes, which form the start and end column
             * segments.
             */
            SetColumnAddress            = 0x15,
            /**
             * @brief Write to framebuffer
             *
             * All data bytes after this command will be written to the region of the framebuffer
             * previously defined by the SetColumnAddress and SetRowAddress commands.
             */
            WriteFramebuffer            = 0x5C,
            /**
             * @brief Set row address range
             *
             * Define the rows (in the framebuffer) that will be modified by an upcoming
             * framebuffer write. The payload is two bytes, which indicate the starting and ending
             * rows to write.
             */
            SetRowAddress               = 0x75,
            /// Set display remap (config for framebuffer)
            SetRemap                    = 0xA0,
            /// Starting line of display
            SetStartLine                = 0xA1,
            /// Display offset
            SetDisplayOffset            = 0xA2,
            /// All pixels on the display are forced off
            AllOffDisplay               = 0xA4,
            /// All pixels on the display are at max brightness
            AllOnDisplay                = 0xA5,
            /// Normal display mode
            NormalDisplay               = 0xA6,
            /// Inverse display
            InvertDisplay               = 0xA7,
            /// Exit partial display mode
            ExitPartialDisplay          = 0xA9,
            /// Configure external function (voltage regulator)
            FunctionSelect              = 0xAB,
            /// Enter sleep mode
            DisplayOff                  = 0xAE,
            /// Exit sleep mode (wake)
            DisplayOn                   = 0xAF,
            /// Configure phase length
            SetPhaseLength              = 0xB1,
            /// Configure clock divider
            SetClockDivider             = 0xB3,
            /// Configure display enhancement
            SetDisplayEnhance           = 0xB4,
            /// Configure GPIO on display driver
            SetGpio                     = 0xB5,
            /// Set precharge period
            SetPrechargePeriod          = 0xB6,
            /// Apply default greyscale map
            ApplyDefaultGreyscale       = 0xB9,
            /// Set pre-charge voltage
            SetPrechargeVoltage         = 0xBB,
            /// Set VCom
            SetVcomH                    = 0xBE,
            /// Write contrast current
            SetContrastCurrent          = 0xC1,
            /// Master current control (valid values are [0, 15])
            SetMasterCurrent            = 0xC7,
            /// Multiplex ratio (duty cycle of driver)
            SetMuxRatio                 = 0xCA,
            /// Configure bonus display enhance
            SetDisplayEnhanceB          = 0xD1,
            /// Disable the command lock on the controller
            SetCommandLock              = 0xFD,
        };

        /// Starting output segment (defined by display module)
        constexpr static const uint8_t kMinSeg{0x1C};
        /// Ending output segment (defined by display module)
        constexpr static const uint8_t kMaxSeg{0x5B};
        /// Starting row (defined by display module)
        constexpr static const uint8_t kMinRow{0};
        /// Ending row (defined by display module)
        constexpr static const uint8_t kMaxRow{63};

    public:
        /// Width of display (pixels)
        constexpr static const size_t kWidth{256};
        /// Height of display (pixels)
        constexpr static const size_t kHeight{64};

        /// Bytes per line
        constexpr static const size_t kStride{kWidth / 2};
        /// Size of the framebuffer, in bytes
        constexpr static const size_t kFramebufferSize{kStride * kHeight};

    public:
        static void Init();

        static int SetBrightness(const float bright);
        static int SetContrast(const float contrast);
        static int SetInverseMode(const bool isInverted);
        static int SetSleepMode(const bool isSleeping);

        static int Transfer();

    public:
        static etl::array<uint8_t, kFramebufferSize> gFramebuffer;

    private:
        static void Configure();

        static int WriteCommand(const Command cmd,
                etl::span<const uint8_t> payload = etl::span<const uint8_t>());
};
}

#endif
