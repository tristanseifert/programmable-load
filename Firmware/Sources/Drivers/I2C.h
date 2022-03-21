#ifndef DRIVERS_I2C_H
#define DRIVERS_I2C_H

#include "SercomBase.h"
#include "I2CBus.h"

#include "Rtos/Rtos.h"

#include <stddef.h>
#include <stdint.h>

#include <etl/span.h>

namespace Drivers {
/**
 * @brief SERCOM I²C driver
 *
 * Implements I²C using one of the SERCOM units. Transfers are interrupt and DMA driven for minimum
 * processor overhead.
 *
 * @remark This currently only implements a master mode, with clock stretching only after the
 *         acknowledge bit. Four wire mode is also not supported, and the peripheral always runs in
 *         32-bit data mode.
 *
 * @remark At this time, the driver only supports single master operation. It is written with the
 *         assumption that it's the only master on the bus.
 */
class I2C: public I2CBus {
    public:
        /**
         * @brief Error codes
         */
        enum Errors: int {
            /**
             * @brief Bus error during transaction
             *
             * An unknown bus error took place during the transaction.
             */
            BusError                    = -100,
            /**
             * @brief No acknowledge received
             *
             * The bus timed out waiting for a device to acknowledge; likely, there is no device
             * at the specified address.
             */
            NoAck                       = -101,
            /**
             * @brief Driver already in use
             *
             * The driver is in use by another task.
             */
            InUse                       = -102,
            /**
             * @brief Not enabled
             *
             * Attempting to perform transactions on a disabled bus
             */
            Disabled                    = -103,
            /**
             * @brief Invalid transaction
             *
             * One or more specified transactions are invalid.
             */
            InvalidTransaction          = -104,
            /**
             * @brief Received NACK
             *
             * The device responded with a NACK unexpectedly
             */
            UnexpectedNAck              = -105,
            /**
             * @brief Reception error
             *
             * Something went wrong during byte reception.
             */
            ReceptionError              = -106,
            /**
             * @brief Transmission error
             *
             * Something went wrong while sending data to the slave.
             */
            TransmissionError           = -107,

            /**
             * @brief Unspecified error
             *
             * Something went seriously wrong inside the driver.
             */
            UnspecifiedError            = -199,
        };

        /**
         * @brief I²C driver configuration
         *
         * Encapsulates the configuration of the I²C peripheral.
         */
        struct Config {
            /**
             * @brief SCL low timeout
             *
             * Abort transactions if SCL is held low for more than approximately 25-35ms, usually
             * as a result of a stuck bus. The current transaction will be aborted, and a STOP
             * condition transmitted by the hardware.
             */
            uint8_t sclLowTimeout:1{0};

            /**
             * @brief Enable DMA operation
             *
             * It's possible to disable DMA operation, relying instead on polled MMIO accesses
             * for the entire transfer.
             *
             * @remark This does not mean that _all_ transfers will use DMA; the driver will only
             *         use DMA if the transfers are large enough that the overhead of setting up
             *         these transfers.
             */
            uint8_t useDma:1{1};

            /**
             * @brief Desired bus frequency
             *
             * This decides the frequency of the I²C bus in Hz; values up to 3.4MHz (for high speed
             * mode) can be specified.
             *
             * Additionally, the bus frequency decides the filters/drive mode that the peripheral
             * operates in:
             *
             * - ≤ 100kHz: Standard mode
             * - ≤ 400kHz: Fast mode
             * - ≤ 1MHz: Fast mode plus
             * – ≤ 3.4MHz: High-speed mode (though the special protocol isn't supported yet)
             */
            uint32_t frequency{100'000};
        };

    public:
        I2C(const SercomBase::Unit unit, const Config &conf);
        virtual ~I2C();

        void reset();
        void enable();
        void disable();

        virtual int perform(etl::span<const Transaction> transactions) override;

    private:
        void waitSysOpSync();

        void irqHandler();
        void irqCompleteTxn(const int status, BaseType_t *woken);
        void beginTransaction(const Transaction &txn, const bool needsStop = true);

        /**
         * @brief Issue a repeated START
         *
         * In the case of a master read, an acknowledge may be sent at this time as well. For
         * master writes, only a STOP condition is sent.
         */
        inline void issueReStart() {
            this->regs->CTRLB.reg |= SERCOM_I2CM_CTRLB_CMD(0x01);
            this->waitSysOpSync();
        }

        /**
         * @brief Issue a STOP condition on the bus
         *
         * In the case of a master read, an acknowledge may be sent at this time as well. For
         * master writes, only a STOP condition is sent.
         */
        inline void issueStop() {
            this->regs->CTRLB.reg |= SERCOM_I2CM_CTRLB_CMD(0x03);
            this->waitSysOpSync();
        }

        static void ApplyConfiguration(const SercomBase::Unit unit, ::SercomI2cm *regs,
                const Config &conf);
        static void UpdateFreq(const SercomBase::Unit unit,
                ::SercomI2cm *regs, const uint32_t frequency);

    private:
        enum class State: uint8_t {
            Idle,
            SendAddress,
            ReadData,
            WriteData,
        };

    private:
        /// Unit number
        SercomBase::Unit unit;
        /// is the device enabled?
        bool enabled{false};
        /// is DMA enabled?
        bool dmaCapable{false};

        /// Internal state machine state
        State state{State::Idle};

        /// MMIO register base
        ::SercomI2cm *regs;

        /**
         * @brief Task currently waiting on bus transactions
         *
         * This is updated whenever we start a new transaction, and it references the task that
         * is blocked on the transactions
         */
        TaskHandle_t waiting;
        /**
         * @brief Completion code of the most recent batch of transactions
         *
         * Set whenever we reach the end of a set of transactions; it is 0 if all transactions ran
         * to completion, otherwise, an error code.
         */
        int completion{0};

        /**
         * @brief Currently executing transaction block
         *
         * Buffer containing the transactions that are currently being executed, or a pointer to
         * `nullptr`
         */
        etl::span<const Transaction> currentTxns;
        /**
         * @brief Currently processing transaction
         *
         * Index of the transaction we're currently processing in currentTxns.
         */
        size_t currentTxn{0};
        /**
         * @brief Byte offset into current transaction
         *
         * Number of bytes we've already copied in the currently executing transaction. When this
         * is equal to the total number of bytes in the transaction, we'll advance to the next
         * transaction.
         */
        size_t currentTxnOffset{0};


        /**
         * @brief Bus lock
         *
         * This lock protects access to the bus, in the way of transactions.
         */
        SemaphoreHandle_t busLock;

        /// Storage for allocating bus lock
        StaticSemaphore_t busLockStorage;

    private:
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

        /**
         * @brief System operation sync timeout
         *
         * Number of cycles to wait for the SYSOP bit to synchronize.
         */
        constexpr static const size_t kSysOpSyncTimeout{100};
};
}

#endif
