#ifndef DRIVERS_SPI_H
#define DRIVERS_SPI_H

#include "SercomBase.h"

#include "Log/Logger.h"

#include <stddef.h>
#include <stdint.h>

#include <etl/array.h>
#include <etl/span.h>

namespace Drivers {
/**
 * @brief SERCOM SPI driver
 *
 * Implements SPI via one of the SERCOM peripherals. The driver supports both blocking and DMA
 * driven operation, automatically configurable during setup.
 *
 * @note The driver only supports 8-bit master mode with no address matching. Additionally, all
 *       data transfers are done in 32-bit units.
 */
class Spi {
    public:
        /// Error codes
        enum Errors: int {
            /**
             * @brief Provided buffer was invalid
             *
             * A buffer for a read/write transaction was invalid. It may have an incorrect length,
             * or is missing both the rx and tx chunks.
             */
            InvalidBuffer               = -200,

            /**
             * @brief Invalid transaction specified
             *
             * One or more of the provided transactions could not be processed; the transaction
             * list may also be empty.
             */
            InvalidTransaction          = -201,
        };

        /**
         * @brief SPI peripheral configuration
         *
         * Various configuration parameters to define how this peripheral works and will be
         * initialized. Some of these may be changed after the fact, and are called out
         * specifically.
         */
        struct Config {
            /**
             * @brief Bit order (LSB first)
             *
             * Defines which bit is set first in the output data: when set, the least significant
             * bit is sent first. Otherwise, it is the most significant bit.
             */
            uint32_t lsbFirst:1{0};

            /**
             * @brief Clock polarity
             *
             * Sets the clock's idle level. 0 indicates low, with the leading edge of each clock
             * being the rising edge. 1 indicates high, where the leading edge is a falling edge.
             */
            uint32_t cpol:1{1};

            /**
             * @brif Clock phase
             *
             * Sets on which clock edge the data on the SPI lines is sampled. 0 indicates it's
             * sampled on a leading clock edge, and data changed on the trailing edge.
             */
            uint32_t cpha:1{1};

            /**
             * @brief Enable receiver
             *
             * When this is cleared, data will not be received, only transmitted. This is useful
             * for devices such as displays that don't support readback.
             */
            uint32_t rxEnable:1{1};

            /**
             * @brief Hardware control for chip select
             *
             * Enables hardware control over the single chip select line exposed by the peripheral
             * for an IO pin. It is automatically asserted before a transaction begins, and
             * deasserted after.
             *
             * @remark When enabled, only a single SPI device is supported on the bus. You can
             *         disable this, and manually handle SPI chip selects if needed for more.
             */
            uint32_t hwChipSelect:1{0};

            /**
             * @brief Enable DMA operation
             *
             * It's possible to disable DMA operation, relying instead on polled MMIO accesses
             * for the entire transfer. This reduces performance quite significantly, however, and
             * is not suggested.
             *
             * @remark When enabled, you must configure the channel numbers for transmit and
             *         receive channels.
             */
            uint32_t useDma:1{1};

            /**
             * @brief Transmit DMA channel
             *
             * Channel number for the transmit DMA channel, if DMA is enabled.
             */
            uint32_t dmaChannelTx:5{0};
            /**
             * @brief Transmit DMA priority
             *
             * Set the priority level of DMA transfers to feed the transmit buffer of the SPI.
             */
            uint32_t dmaPriorityTx:2{0};

            /**
             * @brief Pad for data input
             *
             * Specify which of the 4 pads on the SERCOM is used for the input signal, MISO in
             * master operation.
             */
            uint32_t inputPin:2{3};

            /**
             * @brief Use alternate output pinout
             *
             * When set, the alternate output pinout is used, where the data output is at PAD3
             * rather than PAD0.
             */
            uint32_t alternateOutput:1{0};

            /**
             * @brief Baud rate
             *
             * Desired frequency of the SPI clock, in Hz.
             *
             * @remark The exact frequency may not be achievable, based on constraints from the
             *         processor and peripheral clocks. In this case, the frequency is rounded
             *         down.
             */
            uint32_t sckFrequency;
        };

        /**
         * @brief A SPI transaction
         *
         * This is a small encapsulation of a transaction length, and the associated read/write
         * buffers.
         *
         * @note If both transmit and receive buffers are specified, they must both be sufficiently
         *       large to fit the desired number of bytes.
         */
        struct Transaction {
            /// Pointer to buffer to hold receive data
            void *rxBuf{nullptr};
            /// Pointer to buffer holding data to be transmitted
            const void *txBuf{nullptr};
            /// Number of bytes to transfer
            size_t length;
        };

    public:
        Spi(const SercomBase::Unit unit, const Config &conf);

        void reset();
        void enable();

        int perform(etl::span<const Transaction> transactions);

        /**
         * @brief Perform a write to the SPI device
         *
         * Writes the provided data buffer out the SPI peripheral. All received data will be
         * discarded.
         */
        inline int write(etl::span<const uint8_t> buffer) {
            return this->perform({{
                {
                    .txBuf = buffer.data(),
                    .length = buffer.size()
                },
            }});
        }

    private:
        int doPolledTransfer(const Transaction &txn);
        int doDmaTransfer(const Transaction &txn);

        static void ApplyConfiguration(const SercomBase::Unit unit, ::SercomSpi *regs,
                const Config &conf);
        static void UpdateSckFreq(const SercomBase::Unit unit,
                ::SercomSpi *regs, const uint32_t frequency);

        /**
         * @brief Transfer up to 32 bit of data
         *
         * Performs a single polled transfer that may read or write up to 32 bits of data in a
         * single go.
         *
         * @param txPtr Buffer of transmit data
         * @param rxPtr Buffer of receive data
         * @param length Number of bytes to transfer ([0, 3])
         * @param waitTxComplete Whether we should wait for the current SPI transmission to
         *        complete before. This is used when the `length` is different between the
         *        transactions.
         */
        inline void doPolledTransferSingle(const uint8_t* &txPtr, uint8_t* &rxPtr,
                const size_t length, const bool waitTxComplete) {
            // prepare the transmit data
            uint32_t transmit{0};

            if(txPtr) {
                for(size_t i = 0; i < length; i++) {
                    transmit |= static_cast<uint32_t>(*txPtr++) << (i * 8);
                }
            }

            // ensure the previous block is transmitted, before we write the length register
            if(waitTxComplete) {
                while(!this->regs->INTFLAG.bit.TXC){}
            }

            // program length register
            this->regs->LENGTH.reg = SERCOM_SPI_LENGTH_LENEN | SERCOM_SPI_LENGTH_LEN(length);

            size_t timeout{kResetSyncTimeout};
            do {
                REQUIRE(--timeout, "SPI %s timed out", "enable length");
            } while(!!this->regs->SYNCBUSY.bit.LENGTH);

            // write transmit data
            while(!this->regs->INTFLAG.bit.DRE){}
            this->regs->DATA.reg = transmit;

            // decode the receive data, if desired
            if(this->rxEnabled) {
                while(!this->regs->INTFLAG.bit.RXC){}

                // TODO: validate this
                uint32_t rxWord = this->regs->DATA.reg;
                if(rxPtr) {
                    for(size_t i = 0; i < length; i++) {
                        *rxPtr++ = (rxWord & 0xFF);
                        rxWord >>= 8;
                    }
                }
            }
        }

    private:
        /// Unit number
        SercomBase::Unit unit;
        /// is the device enabled?
        bool enabled{false};
        /// is the receiver enabled?
        bool rxEnabled{false};

        /// is DMA enabled?
        uint16_t dmaCapable:1{false};
        /// use DMA for transmit
        uint16_t dmaTx:1{false};
        /// DMA channel for transmit
        uint16_t dmaTxChannel:5{0};
        /// Priority for DMA
        uint16_t dmaTxPriority:2{0};
        /// Use DMA for receive
        uint16_t dmaRx:1{false};
        /// DMA channel for receive
        uint16_t dmaRxChannel:5{0};

        /// MMIO register base
        ::SercomSpi *regs;

        /**
         * @brief DMA transfer size threshold
         *
         * SPI transfers above this size (in bytes) will always be performed with DMA, if possible,
         * to ease the processor overhead.
         *
         * This should be set so that the overhead of configuring the DMA channel is lower than the
         * time it would have taken to do the transfer via polling mode.
         */
        constexpr static const size_t kDmaThreshold{128};

        /**
         * @brief Enable timeout
         *
         * Number of cycles to wait for the enable bit to synchronize. If this timeout expires, we
         * assume something is wrong with the peripheral.
         */
        constexpr static const size_t kEnableSyncTimeout{1000};

        /**
         * @brief Reset timeout
         *
         * Number of cycles to wait for the reset bit to synchronize. If this timeout expires, we
         * assume something is wrong with the peripheral.
         */
        constexpr static const size_t kResetSyncTimeout{1000};

        /// Configure whether we desire extra debug logging
        constexpr static const bool kExtraLogging{false};
};
}

#endif
