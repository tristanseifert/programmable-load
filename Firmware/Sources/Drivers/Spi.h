#ifndef DRIVERS_SPI_H
#define DRIVERS_SPI_H

#include "SercomBase.h"

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
            uint8_t lsbFirst:1{0};

            /**
             * @brief Clock polarity
             *
             * Sets the clock's idle level. 0 indicates low, with the leading edge of each clock
             * being the rising edge. 1 indicates high, where the leading edge is a falling edge.
             */
            uint8_t cpol:1{1};

            /**
             * @brif Clock phase
             *
             * Sets on which clock edge the data on the SPI lines is sampled. 0 indicates it's
             * sampled on a leading clock edge, and data changed on the trailing edge.
             */
            uint8_t cpha:1{1};

            /**
             * @brief Enable receiver
             *
             * When this is cleared, data will not be received, only transmitted. This is useful
             * for devices such as displays that don't support readback.
             */
            uint8_t rxEnable:1{1};

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
            uint8_t hwChipSelect:1{0};

            /**
             * @brief Enable DMA operation
             *
             * It's possible to disable DMA operation, relying instead on polled MMIO accesses
             * for the entire transfer. This reduces performance quite significantly, however, and
             * is not suggested.
             */
            uint8_t useDma:1{1};

            /**
             * @brief Pad for data input
             *
             * Specify which of the 4 pads on the SERCOM is used for the input signal, MISO in
             * master operation.
             */
            uint8_t inputPin:2{3};

            /**
             * @brief Use alternate output pinout
             *
             * When set, the alternate output pinout is used, where the data output is at PAD3
             * rather than PAD0.
             */
            uint8_t alternateOutput:1{0};

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

        static void ApplyConfiguration(const SercomBase::Unit unit, ::SercomSpi *regs,
                const Config &conf);
        static void UpdateSckFreq(const SercomBase::Unit unit,
                ::SercomSpi *regs, const uint32_t frequency);

    private:
        /// Unit number
        SercomBase::Unit unit;
        /// is the device enabled?
        bool enabled{false};
        /// is the receiver enabled?
        bool rxEnabled{false};
        /// is DMA enabled?
        bool dmaCapable{false};

        /// MMIO register base
        ::SercomSpi *regs;

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
};
}

#endif
