#include "Spi.h"
#include "SercomBase.h"
#include "Dma.h"
#include "Gpio.h"

#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include <vendor/sam.h>

using namespace Drivers;

/**
 * @brief Initialize the SERCOM in SPI master mode.
 *
 * @param unit SERCOM unit to use
 * @param conf Configuration for the unit
 *
 * @remark It's assumed the clock to this device has been configured when this constructor is
 *         invoked. All other resources (DMA, interrupts, etc.) are initialized here.
 */
Spi::Spi(const SercomBase::Unit unit, const Config &conf) : unit(unit), rxEnabled(conf.rxEnable),
    regs(&(SercomBase::MmioFor(unit)->SPI)) {
    // mark as used and reset the unit
    SercomBase::MarkAsUsed(unit);
    this->reset();

    // configure DMA, interrupts, then the peripheral
    if(conf.useDma) {
        this->dmaCapable = true;
        this->dmaTx = 1;
        this->dmaTxChannel = conf.dmaChannelTx;
        this->dmaTxPriority = conf.dmaPriorityTx;

        REQUIRE(!conf.rxEnable, "SPI DMA receive not yet implemented");
    }

    // TODO: IRQ config

    ApplyConfiguration(this->unit, this->regs, conf);

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
    // cancel DMA
    if(this->dmaTx) {
        Dma::ResetChannel(this->dmaTxChannel);
    }
    if(this->dmaRx) {
        Dma::ResetChannel(this->dmaRxChannel);
    }

    // reset the peripheral and wait
    taskENTER_CRITICAL();

    this->regs->CTRLA.reg = SERCOM_SPI_CTRLA_SWRST;
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

    // enable DMA
    if(this->dmaTx) {
        Dma::ConfigureChannel(this->dmaTxChannel, Dma::FifoThreshold::x1, 0,
                Dma::TriggerAction::Burst, SercomBase::GetDmaTxTrigger(this->unit),
                this->dmaTxPriority);
    }

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
 * @brief Perform one or more SPI transactions
 *
 * Iterates over the provided list of transfer descriptors, and executes them sequentially. For
 * each descriptor, the driver selects whether to use polled or DMA mode, if configured.
 *
 * @param transactions Transfer descriptors for the peripheral
 *
 * @return 0 on success, negative error code otherwise
 */
int Spi::perform(etl::span<const Transaction> transactions) {
    int err;

    // validate inputs
    if(transactions.empty()) {
        return Errors::InvalidTransaction;
    }

    // perform each transaction
    for(const auto &txn : transactions) {
        if(this->dmaCapable && txn.length > kDmaThreshold) {
            err = this->doDmaTransfer(txn);
        } else {
            err = this->doPolledTransfer(txn);
        }

        // abort if this transaction failed
        if(err) {
            goto beach;
        }
    }

    // clean up and return last error code
beach:;
    return err;
}

/**
 * @brief Perform an SPI transfer using DMA
 *
 * @param txn Transaction to perform
 *
 * @return 0 on success, negative error code otherwise
 */
int Spi::doDmaTransfer(const Transaction &txn) {
    int err{0};
    bool tx{false};

    // validate arguments
    if(!txn.length || (!txn.rxBuf && !txn.txBuf)) {
        return Errors::InvalidBuffer;
    }

    auto rxPtr = reinterpret_cast<uint8_t *>(txn.rxBuf);
    auto txPtr = reinterpret_cast<const uint8_t *>(txn.txBuf);

    const auto dmaLength = txn.length & ~3;
    if(!dmaLength) {
        return Errors::InvalidBuffer;
    }

    // disable length register (DMA always writes the whole 32 bits)
    this->regs->LENGTH.reg = 0;

    size_t timeout{kResetSyncTimeout};
    do {
        REQUIRE(--timeout, "SPI %s timed out", "disable length");
    } while(!!this->regs->SYNCBUSY.bit.LENGTH);

    // configure DMA channels
    if(txPtr) {
        err = Dma::ConfigureTransfer(this->dmaTxChannel, Dma::BeatSize::Word, txPtr, true,
                const_cast<uint32_t *>(&this->regs->DATA.reg), false, dmaLength);
        if(err) {
            goto beach;
        }
        tx = true;
    }
    if(this->rxEnabled && rxPtr) {
        // TODO: configure receive DMA
        Logger::Panic("SPI: %s", "DMA receive not yet implemented");
    }

    // start DMA channels (and wait for them to complete)
    if(tx) {
        Dma::EnableChannel(this->dmaTxChannel);

        Dma::WaitForCompletion(this->dmaTxChannel);
    }

    // transfer remaining bytes
    if(txn.length != dmaLength) {
        // update read/write pointers
        const auto remaining = txn.length - dmaLength;

        if(txPtr) {
            txPtr += dmaLength;
        }
        if(rxPtr) {
            rxPtr += dmaLength;
        }

        // perform a single polled mode transfer
        taskENTER_CRITICAL();
        this->doPolledTransferSingle(txPtr, rxPtr, remaining, true);
        taskEXIT_CRITICAL();
    }

    // clean up
beach:;
    // disable DMA channels
    if(tx) {
        Dma::DisableChannel(this->dmaTxChannel);
    }

    return err;
}

/**
 * @brief Perform a SPI transfer in polled mode
 *
 * @param txn Transaction to perform
 *
 * @return 0 on success, negative error code otherwise
 *
 * @remark Either of the buffers may be `nullptr`, and we'll transmit either 0's or discard the
 *         received data.
 *
 * @remark Chip select should be handled by higher level functions, unless hardware chip select is
 *         used.
 */
int Spi::doPolledTransfer(const Transaction &txn) {
    // validate arguments
    if(!txn.length || (!txn.rxBuf && !txn.txBuf)) {
        return Errors::InvalidBuffer;
    }

    auto rxPtr = reinterpret_cast<uint8_t *>(txn.rxBuf);
    auto txPtr = reinterpret_cast<const uint8_t *>(txn.txBuf);

    // process as many bytes as provided
    taskENTER_CRITICAL();

    // send 32 bits at a time
    const auto blocks = txn.length / 4;

    if(blocks) {
        // disable length register (write all 32 bits)
        this->regs->LENGTH.reg = 0;

        size_t timeout{kResetSyncTimeout};
        do {
            REQUIRE(--timeout, "SPI %s timed out", "disable length");
        } while(!!this->regs->SYNCBUSY.bit.LENGTH);

        // process each block
        for(size_t i = 0; i < blocks; i++) {
            // wait for TXRDY and write out
            while(!this->regs->INTFLAG.bit.DRE){}

            if(txPtr) {
                this->regs->DATA.reg =
                    (txPtr[0] | (txPtr[1] << 8) | (txPtr[2] << 16) | (txPtr[3] << 24));
                txPtr += 4;
            } else {
                this->regs->DATA.reg = 0;
            }

            // if receiver is enabled, wait for data and read it
            if(this->rxEnabled) {
                while(!this->regs->INTFLAG.bit.RXC){}

                const uint32_t rxWord = this->regs->DATA.reg;
                if(rxPtr) {
                    *rxPtr++ = rxWord & 0xFF;
                    *rxPtr++ = (rxWord >> 8) & 0xFF;
                    *rxPtr++ = (rxWord >> 16) & 0xFF;
                    *rxPtr++ = (rxWord >> 24) & 0xFF;
                }
            }
        }
    }

    // set the length register for the remaining bytes, if any
    const auto remaining = txn.length - (blocks * 4);

    if(remaining) {
        this->doPolledTransferSingle(txPtr, rxPtr, remaining, !!blocks);
    }

    // wait for last byte to finish transmitting
    while(!this->regs->INTFLAG.bit.TXC){}

    taskEXIT_CRITICAL();

    return 0;
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
void Spi::ApplyConfiguration(const SercomBase::Unit unit, ::SercomSpi *regs, const Config &conf) {
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

    if(kExtraLogging) Logger::Debug("SERCOM%u %s %s: $%08x", static_cast<unsigned int>(unit),
            "SPI", "CTRLA", temp);
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

    if(kExtraLogging) Logger::Debug("SERCOM%u %s %s: $%08x", static_cast<unsigned int>(unit),
            "SPI", "CTRLB", temp);
    regs->CTRLB.reg = temp & SERCOM_SPI_CTRLB_MASK;

    /**
     * CTRLC: Control C
     *
     * Use 32-bit data register transfers (meaning we must also load LENGTH each time) and disable
     * all inter-character spacing.
     */
    temp = SERCOM_SPI_CTRLC_DATA32B | SERCOM_SPI_CTRLC_ICSPACE(0);

    if(kExtraLogging) Logger::Debug("SERCOM%u %s %s: $%08x", static_cast<unsigned int>(unit),
            "SPI", "CTRLC", temp);
    regs->CTRLC.reg = temp & SERCOM_SPI_CTRLC_MASK;

    // then last, calculate the correct baud rate
    UpdateSckFreq(unit, regs, conf.sckFrequency);
}

/**
 * @brief Sets the SPI clock frequency
 *
 * @param unit SERCOM unit
 * @param regs Registers to receive the configuration
 * @param frequency Desired frequency, in Hz
 *
 * @remark If the exact frequency cannot be achieved, the calculation will round down.
 */
void Spi::UpdateSckFreq(const SercomBase::Unit unit, ::SercomSpi *regs,
        const uint32_t frequency) {
    const auto core = SercomBase::CoreClockFor(unit);
    REQUIRE(core, "SERCOM%u: core clock unknown", static_cast<unsigned int>(unit));

    const auto baud = (core / (2 * frequency)) - 1;
    const auto actual = core / (2 * (baud + 1));

    REQUIRE(baud <= 0xFF, "SPI baud rate out of range (%u Hz = $%08x)", frequency, baud);

    if(kExtraLogging) {
        Logger::Debug("SERCOM%u SPI freq: request %u Hz, got %u Hz",
                static_cast<unsigned int>(unit), frequency, actual);
    }
    regs->BAUD.reg = baud;
}

