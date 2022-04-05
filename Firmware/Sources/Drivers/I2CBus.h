#ifndef DRIVERS_I2CBUS_H
#define DRIVERS_I2CBUS_H

#include <stddef.h>
#include <stdint.h>

#include <etl/span.h>

namespace Drivers {
/**
 * @brief Abstract interface for an I²C bus
 *
 * This interface provides basic and high level methods for interacting with devices on the I²C
 * bus. Abstracting this into an interface allows device drivers to work the same, whether they are
 * connected directly to the bus, or behind some sort of multiplexer.
 */
class I2CBus {
    public:
        /**
         * @brief A single transaction on the I²C bus
         *
         * Transactions address a single device, and consist of either a read or a write of data;
         * you may have multiple transactions back-to-back, without re-addressing the device in
         * the middle.
         */
        struct Transaction {
            /**
             * @brief Device address
             *
             * This is the 7-bit device address. It is shifted left one bit to accomodate the
             * read/write bit.
             */
            uint8_t address;

            /**
             * @brief Read/write bit
             *
             * Set this bit to perform a read transaction, clear it to write.
             */
            uint8_t read:1{0};

            /**
             * @brief Continuation from last transaction
             *
             * When set, this transaction is a continuation of the last one. The bus will not be
             * relinquished, and a repeated START is used instead of a STOP, START sequence.
             */
            uint8_t continuation:1{0};

            /**
             * @brief Skip restart
             *
             * If the transaction is a continuation from the previous, and this bit is set, we do
             * not generate another start condition. This is useful for splitting data in the
             * same transaction across multiple buffers.
             */
            uint8_t skipRestart:1{0};

            /**
             * @brief Transfer length
             *
             * Total number of bytes to transfer in this transaction
             */
            uint16_t length{0};

            /**
             * @brief Transfer buffer
             *
             * A buffer that contains data to be transmitted (write) or received (read.)
             *
             * @note This buffer must be at least length bytes when both receiving and transmitting
             *       but it may be larger.
             */
            etl::span<uint8_t> data;
        };

    public:
        virtual ~I2CBus() = default;

        /**
         * @brief Execute a series of transactions on the bus.
         *
         * Forwards to the driver one or more transactions (which are performed back-to-back on the
         * bus) to perform. The call returns when all transactions have completed, or when there is
         * a failure.
         *
         * @param transactions An array of transaction descriptors
         *
         * @return 0 on success, or a negative error code.
         *
         * @remark The implementation should guarantee that all transactions specified complete as
         *         one atomic unit. For example, if the bus is behind a multiplexer, it should not
         *         switch busses until all transactions submitted on one bus are done.
         *
         * @remark All transactions (and their underlying buffers) must be valid until this call
         *         returns. The buffer memory of each transaction must be in memory accessible to
         *         peripherals.
         */
        virtual int perform(etl::span<const Transaction> transactions) = 0;

    protected:
        static int ValidateTransactions(etl::span<const Transaction> transactions);
};
}

#endif
