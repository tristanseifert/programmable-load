#include "Display.h"
#include "../Hardware.h"

#include "Drivers/Spi.h"
#include "Gfx/Framebuffer.h"
#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include <string.h>
#include <etl/algorithm.h>
#include <etl/array.h>

using namespace App::Pinball;

etl::array<uint8_t, Display::kFramebufferSize> Display::gFramebuffer;

/*
 * Export the default system framebuffer.
 *
 * This belongs to the graphics library, but for convenience, we declare it here where we have the
 * actual underlying memory of the framebuffer defined as well.
 */
Gfx::Framebuffer Gfx::Framebuffer::gMainBuffer{
    .size = {
        .width = Display::kWidth,
        .height = Display::kHeight,
    },
    .data = Display::gFramebuffer,
    .stride = Display::kStride,
};


/**
 * @brief Initialize the display driver
 *
 * This sends to the display the initialization sequence, clears the framebuffer, then transfers
 * that to the display.
 *
 * @remark If the /RESET line is connected to the display, it should have been toggled before this
 *         call is made.
 */
void Display::Init() {
    // send the initialization sequence
    Configure();

    // clear framebuffer and transfer
    etl::fill(gFramebuffer.begin(), gFramebuffer.end(), 0);
    Transfer();

    // turn the display on (exit sleep mode)
    SetSleepMode(false);
}

/**
 * @brief Send the initialization sequence to the display
 *
 * All registers in the display controller will be programmed, and the display will be in regular
 * mode, ready to display data.
 *
 * @remark The display will not yet be enabled: call SetSleepMode after initializing it. This
 *         allows transferring a clean framebuffer before displaying the contents.
 */
void Display::Configure() {
    int err;

    /*
     * Configure the display controller.
     *
     * Most of these values, and the whole sequence, were taken from the vendor provided
     * [example code](http://www.buydisplay.com/download/democode/ER-OLEDM032-1_DemoCode.txt) and
     * seem to work alright.
     */
    err = WriteCommand(Command::SetCommandLock, {{ 0x12 }});
    REQUIRE(!err, "%s: failed to set %s (%d)", "SSD1322", "cmd lock", err);

    err = WriteCommand(Command::DisplayOff);
    REQUIRE(!err, "%s: failed to set %s (%d)", "SSD1322", "display off", err);

    err = WriteCommand(Command::SetClockDivider, {{ 0x91 }});
    REQUIRE(!err, "%s: failed to set %s (%d)", "SSD1322", "clock", err);

    err = WriteCommand(Command::SetMuxRatio, {{ 0x3F }});
    REQUIRE(!err, "%s: failed to set %s (%d)", "SSD1322", "clock", err);

    err = WriteCommand(Command::SetDisplayOffset, {{ 0x00 }});
    REQUIRE(!err, "%s: failed to set %s (%d)", "SSD1322", "display offset", err);
    err = WriteCommand(Command::SetStartLine, {{ 0x00 }});
    REQUIRE(!err, "%s: failed to set %s (%d)", "SSD1322", "start line", err);

    // h addr increment, disable col addr remap, enable nibble remap, scan from COM[N-1] to COM,
    // no COM split; enable dual COM mode
    err = WriteCommand(Command::SetRemap, {{ 0x14, 0x11 }});
    REQUIRE(!err, "%s: failed to set %s (%d)", "SSD1322", "remap", err);

    err = WriteCommand(Command::SetGpio, {{ 0x00 }});
    REQUIRE(!err, "%s: failed to set %s (%d)", "SSD1322", "gpio", err);

    // use external VDD
    err = WriteCommand(Command::FunctionSelect, {{ 0x01 }});
    REQUIRE(!err, "%s: failed to set %s (%d)", "SSD1322", "function select", err);

    // enable external Vsl; enhanced low GS display quality (default is 0xb5)
    err = WriteCommand(Command::SetDisplayEnhance, {{ 0xA0, 0xFD }});
    REQUIRE(!err, "%s: failed to set %s (%d)", "SSD1322", "display enhance", err);

    // configure contrast
    err = WriteCommand(Command::SetContrastCurrent, {{ 0xFF }});
    REQUIRE(!err, "%s: failed to set %s (%d)", "SSD1322", "contrast current", err);

    err = WriteCommand(Command::SetMasterCurrent, {{ 0x0F }});
    REQUIRE(!err, "%s: failed to set %s (%d)", "SSD1322", "master current", err);

    // configure greyscale map
    err = WriteCommand(Command::ApplyDefaultGreyscale);
    REQUIRE(!err, "%s: failed to set %s (%d)", "SSD1322", "default greyscale", err);

    // more display configuration
    err = WriteCommand(Command::SetPhaseLength, {{ 0xE2 }});
    REQUIRE(!err, "%s: failed to set %s (%d)", "SSD1322", "phase length", err);

    err = WriteCommand(Command::SetDisplayEnhanceB, {{ 0x82, 0x20 }});
    REQUIRE(!err, "%s: failed to set %s (%d)", "SSD1322", "display enhance 2", err);

    err = WriteCommand(Command::SetPrechargeVoltage, {{ 0x1F }});
    REQUIRE(!err, "%s: failed to set %s (%d)", "SSD1322", "Vprecharge", err);

    err = WriteCommand(Command::SetPrechargePeriod, {{ 0x08 }});
    REQUIRE(!err, "%s: failed to set %s (%d)", "SSD1322", "second precharge period", err);

    err = WriteCommand(Command::SetVcomH, {{ 0x07 }});
    REQUIRE(!err, "%s: failed to set %s (%d)", "SSD1322", "VcomH", err);

    // put the display into normal display mode
    err = WriteCommand(Command::NormalDisplay);
    REQUIRE(!err, "%s: failed to set %s (%d)", "SSD1322", "display normal", err);

    err = WriteCommand(Command::ExitPartialDisplay);
    REQUIRE(!err, "%s: failed to set %s (%d)", "SSD1322", "exit partial display", err);
}

/**
 * @brief Transfer the framebuffer to the display.
 */
int Display::Transfer() {
    int err;

    // set up the write
    static const etl::array<const uint8_t, 2> kColumnAddr{{kMinSeg, kMaxSeg}};
    err = WriteCommand(Command::SetColumnAddress, kColumnAddr);
    if(err) {
        return err;
    }

    static const etl::array<const uint8_t, 2> kRowAddr{{kMinRow, kMaxRow}};
    err = WriteCommand(Command::SetRowAddress, kRowAddr);
    if(err) {
        return err;
    }

    // transfer the framebuffer itself
    return WriteCommand(Command::WriteFramebuffer, gFramebuffer);
}


/**
 * @brief Set whether the display contents are inverted
 *
 * @param isInverted Whether the display is inverted on the panel output
 */
int Display::SetInverseMode(const bool isInverted) {
    if(isInverted) {
        return WriteCommand(Command::InvertDisplay);
    } else {
        return WriteCommand(Command::NormalDisplay);
    }
}

/**
 * @brief Set the display's power mode.
 *
 * When in sleep mode, the display is not actively driven, and the controller enters a low power
 * state.
 *
 * @param isSleeping Whether the display should enter sleep mode or wake up
 */
int Display::SetSleepMode(const bool isSleeping) {
    if(isSleeping) {
        return WriteCommand(Command::DisplayOff);
    } else {
        return WriteCommand(Command::DisplayOn);
    }
}



/**
 * @brief Write a command (with optional payload) to the display
 *
 * Sends the command byte, with the command strobe asserted, followed by zero or more data bytes,
 * with the command strobe deasserted.
 *
 * @param cmd Command to send
 * @param payload Data payload to send, if non-zero length
 *
 * @return 0 on success, error code otherwise
 */
int Display::WriteCommand(const Command cmd, etl::span<const uint8_t> payload) {
    int err;

    // send the command byte (with asserted command strobe)
    Hw::SetDisplayDataCommandFlag(false);
    Hw::SetDisplaySelect(true);

    etl::array<uint8_t, 1> cmdBuf{{static_cast<uint8_t>(cmd)}};
    err = Hw::gDisplaySpi->write(cmdBuf);
    if(err) {
        goto beach;
    }

    // send payload (with deasserted command strobe)
    if(!payload.empty()) {
        Hw::SetDisplayDataCommandFlag(true);
        err = Hw::gDisplaySpi->write(payload);
    }

    // deassert /CS
beach:;
    Hw::SetDisplaySelect(false);
    return err;
}

