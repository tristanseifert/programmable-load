#include "Spi.h"
#include "SercomBase.h"
#include "Gpio.h"

#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include <vendor/sam.h>

using namespace Drivers;

extern "C" uint32_t SystemCoreClock;

/**
 * @brief Initialize the SERCOM in SPI master mode.
 *
 * @param unit SERCOM unit to use
 * @param conf Configuration for the unit
 *
 * @remark It's assumed the clock to this device has been configured when this constructor is
 *         invoked. All other resources (DMA, interrupts, etc.) are initialized here.
 */
Spi::Spi(const SercomBase::Unit unit, const Config &conf) : unit(unit),
    regs(&(SercomBase::MmioFor(unit)->SPI)) {
    // mark as used and reset the unit
    SercomBase::MarkAsUsed(unit);
    this->reset();

    // configure DMA, interrupts, then the peripheral
    if(conf.useDma) {
        // TODO: configure DMAC
        this->dmaCapable = true;
    }

    // TODO: IRQ config

    ApplyConfiguration(this->regs, conf);

    // enable the peripheral
    this->enable();
}

/**
 * @brief Reset the peripheral
 *
 * All registers will be reset to their default values, and the SERCOM is disabled.
 *
 * @note Any in progress DMA transfers will also be canceled, so data loss may result.
 */
void Spi::reset() {
    // TODO: cancel DMA

    // TODO: disable IRQs

    // reset the peripheral and wait
    taskENTER_CRITICAL();

    this->regs->CTRLA.reg |= SERCOM_SPI_CTRLA_SWRST;
    size_t timeout{kResetSyncTimeout};
    do {
        REQUIRE(--timeout, "SPI %s timed out", "reset");
    } while(!!this->regs->SYNCBUSY.bit.SWRST);

    this->enabled = false;
    taskEXIT_CRITICAL();
}

/**
 * @brief Enable the peripheral
 *
 * Invoke this once the device is configured, so that it can perform transactions.
 */
void Spi::enable() {
    REQUIRE(!this->enabled, "SPI already enabled");

    // set the bit
    taskENTER_CRITICAL();

    this->regs->CTRLA.reg |= SERCOM_SPI_CTRLA_ENABLE;
    size_t timeout{kEnableSyncTimeout};
    do {
        REQUIRE(--timeout, "SPI %s timed out", "enable");
    } while(!!this->regs->SYNCBUSY.bit.ENABLE);

    this->enabled = true;
    taskEXIT_CRITICAL();
}

/**
 * @brief Configure the SERCOM SPI registers based on the provided configuration.
 *
 * @param regs Registers to receive the configuration
 * @param conf Peripheral configuration
 *
 * @remark The peripheral should be disabled when invoking this; it's best to perform a reset
 *         before.
 */
void Spi::ApplyConfiguration(::SercomSpi *regs, const Config &conf) {
    uint32_t temp{0};

    /*
     * CTRLA: Control A
     *
     * Copy from configuration: data order, clock polarity & phase, pinout
     *
     * Fixed: SPI master mode, regular SPI frame mode
     */
    if(conf.lsbFirst) {
        temp |= SERCOM_SPI_CTRLA_DORD;
    }
    if(conf.cpol) {
        temp |= SERCOM_SPI_CTRLA_CPOL;
    }
    if(conf.cpha) {
        temp |= SERCOM_SPI_CTRLA_CPHA;
    }

    temp |= SERCOM_SPI_CTRLA_DOPO(conf.alternateOutput ? 0x2 : 0x0);
    temp |= SERCOM_SPI_CTRLA_DIPO(conf.inputPin);

    temp |= SERCOM_SPI_CTRLA_MODE(static_cast<uint8_t>(SercomBase::Mode::SpiMaster));

    Logger::Debug("Sercom %p %s: $%08x", regs, "CTRLA", temp);
    regs->CTRLA.reg = temp & SERCOM_SPI_CTRLA_MASK;

    /*
     * CTRLB: Control B
     *
     * Copy from configuration: Receiver enable, master slave select enable
     *
     * Fixed: Address mode (disabled), preload data register, 8-bit characters
     */
    temp = 0;

    if(conf.rxEnable) {
        temp |= SERCOM_SPI_CTRLB_RXEN;
    }
    if(conf.hwChipSelect) {
        temp |= SERCOM_SPI_CTRLB_MSSEN;
    }

    temp |= SERCOM_SPI_CTRLB_AMODE(0);
    temp |= SERCOM_SPI_CTRLB_CHSIZE(0x0);

    temp |= SERCOM_SPI_CTRLB_PLOADEN; // preload data register

    Logger::Debug("Sercom %p %s: $%08x", regs, "CTRLB", temp);
    regs->CTRLB.reg = temp & SERCOM_SPI_CTRLB_MASK;

    /**
     * CTRLC: Control C
     *
     * Use 32-bit data register transfers (meaning we must also load LENGTH each time) and disable
     * all inter-character spacing.
     */
    temp = SERCOM_SPI_CTRLC_DATA32B | SERCOM_SPI_CTRLC_ICSPACE(0);

    Logger::Debug("Sercom %p %s: $%08x", regs, "CTRLC", temp);
    regs->CTRLC.reg = temp & SERCOM_SPI_CTRLC_MASK;

    // then last, calculate the correct baud rate
    UpdateSckFreq(regs, conf.sckFrequency);
}

/**
 * @brief Sets the SPI clock frequency
 *
 * @param regs Registers to receive the configuration
 * @param frequency Desired frequency, in Hz
 *
 * @remark If the exact frequency cannot be achieved, the calculation will round down.
 *
 * @remark This assumes that the baud rate reference clock is the same as the system core clock.
 * @todo Determine if we can better figure out the input clock
 */
void Spi::UpdateSckFreq(::SercomSpi *regs, const uint32_t frequency) {
    const auto baud = (SystemCoreClock / (2 * frequency)) - 1;
    const auto actual = SystemCoreClock / (2 * (baud + 1));

    REQUIRE(baud <= 0xFF, "SPI baud rate out of range (%u Hz = $%08x)", frequency, baud);

    Logger::Debug("Sercom %p freq: request %u Hz, got %u Hz", regs, frequency, actual);
    regs->BAUD.reg = baud;
}

