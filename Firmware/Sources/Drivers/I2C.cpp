#include "Common.h"
#include "I2C.h"
#include "I2CBus.h"
#include "SercomBase.h"

#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include <vendor/sam.h>

using namespace Drivers;

/**
 * @brief Initialize the I²C master on the given SERCOM instance
 *
 * Configures the I²C per the specified configuration.
 *
 * @param unit The SERCOM unit to use
 * @param conf Configuration to apply
 */
I2C::I2C(const SercomBase::Unit unit, const Config &conf) : unit(unit),
    regs(&(SercomBase::MmioFor(unit)->I2CM)) {
    // mark the underlying SERCOM as used
    SercomBase::MarkAsUsed(unit);

    // allocate locks
    this->busLock = xSemaphoreCreateMutexStatic(&this->busLockStorage);

    // reset
    this->reset();

    // TODO: configure DMA
    if(conf.useDma) {
        // TODO: configure DMAC
        this->dmaCapable = true;
    }

    /*
     * Set up interrupts
     *
     * Enable error (bus errors, timeouts, etc) slave on bus (byte successfully received) and
     * master on bus (byte successfully transmitted)
     */
    NVIC_SetPriority(SercomBase::GetIrqVector(unit, 0),
            configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 2);

    SercomBase::RegisterHandler(unit, 0, [](void *ctx) {
        reinterpret_cast<I2C *>(ctx)->irqHandler();
    }, this);

    this->regs->INTENSET.reg = SERCOM_I2CM_INTENSET_MB | SERCOM_I2CM_INTENSET_SB |
        SERCOM_I2CM_INTENSET_ERROR;

    // apply configuration
    ApplyConfiguration(unit, this->regs, conf);

    // enable the peripheral
    this->enable();
}

/**
 * @brief Deinitializes the I²C master and resets the peripheral.
 */
I2C::~I2C() {
    // disable, then reset hardware (also disables IRQs)
    this->disable();
    this->reset();

    // close locks
    vSemaphoreDelete(this->busLock);

    // mark as available
    SercomBase::MarkAsAvailable(this->unit);
}

/**
 * @brief Reset the peripheral
 *
 * All registers will be reset to their default values, and the SERCOM is disabled.
 *
 * @note Any in progress transfers will also be canceled, so data loss may result.
 */
void I2C::reset() {
    // TODO: cancel DMA

    // disable IRQs in NVIC
    NVIC_DisableIRQ(SercomBase::GetIrqVector(this->unit, 0));

    // reset the peripheral and wait
    taskENTER_CRITICAL();

    this->regs->CTRLA.reg = SERCOM_I2CM_CTRLA_SWRST;
    size_t timeout{kResetSyncTimeout};
    do {
        REQUIRE(--timeout, "I2C %s timed out", "reset");
    } while(!!this->regs->SYNCBUSY.bit.SWRST);

    this->enabled = false;
    taskEXIT_CRITICAL();
}

/**
 * @brief Enable the peripheral
 *
 * Invoke this once the device is configured, so that it can perform transactions.
 */
void I2C::enable() {
    REQUIRE(!this->enabled, "I2C already enabled");

    // set the bit
    taskENTER_CRITICAL();

    this->regs->CTRLA.reg |= SERCOM_I2CM_CTRLA_ENABLE;
    size_t timeout{kEnableSyncTimeout};
    do {
        REQUIRE(--timeout, "I2C %s timed out", "enable");
    } while(!!this->regs->SYNCBUSY.bit.ENABLE);

    this->enabled = true;

    // force state machine into IDLE state
    this->regs->STATUS.reg = SERCOM_I2CM_STATUS_BUSSTATE(0b01);
    this->waitSysOpSync();

    // enable interrupts
    NVIC_EnableIRQ(SercomBase::GetIrqVector(this->unit, 0));
    taskEXIT_CRITICAL();
}

/**
 * @brief Disable the peripheral
 *
 * Invoke this once the device is configured, so that it can perform transactions.
 */
void I2C::disable() {
    REQUIRE(this->enabled, "I2C already disabled");

    // TODO: cancel transactions in progress

    // disable interrupts
    taskENTER_CRITICAL();
    NVIC_DisableIRQ(SercomBase::GetIrqVector(this->unit, 0));

    // set the bit
    this->regs->CTRLA.reg &= ~SERCOM_I2CM_CTRLA_ENABLE;
    size_t timeout{kEnableSyncTimeout};
    do {
        REQUIRE(--timeout, "I2C %s timed out", "enable");
    } while(!!this->regs->SYNCBUSY.bit.ENABLE);

    this->enabled = false;
    taskEXIT_CRITICAL();
}

/**
 * @brief Wait for system operation register synchronization
 *
 * Ensures that any writes to CTRLB.CMD, STATUS.BUSSTATE, ADDR or DATA have been posted to the
 * device, as they require synchronization when the device is enabled.
 */
void I2C::waitSysOpSync() {
    size_t timeout{kSysOpSyncTimeout};
    do {
        REQUIRE(--timeout, "I2C %s timed out", "SYSOP");
    } while(!!this->regs->SYNCBUSY.bit.SYSOP);
}



/**
 * @brief Perform bus transactions
 *
 * Execute the provided transactions, one after another.
 */
int I2C::perform(etl::span<const Transaction> transactions) {
    int err{-1};
    uint32_t note{0};
    BaseType_t ok;

    // ensure we're enabled
    if(!this->enabled) {
        return Errors::Disabled;
    }

    // validate inputs
    if(transactions.empty()) {
        return Errors::InvalidTransaction;
    }

    err = I2CBus::ValidateTransactions(transactions);
    if(err) {
        return err;
    }

    // acquire lock on the bus and prepare for the first transaction
    if(!xSemaphoreTake(this->busLock, portMAX_DELAY)) {
        // failed to acquire lock
        return Errors::InUse;
    }

    this->waiting = xTaskGetCurrentTaskHandle();
    this->completion = -1;
    this->state = State::Idle;
    this->currentTxn = 0;
    this->currentTxnOffset = 0;
    this->currentTxns = transactions;

    /*
     * Start the transfer by writing the address of the first device.
     *
     * In the case of a read, also preprogram CTRLB.ACKACT so that if we're reading only a single
     * byte, we transmit a NACK automatically once that first byte has been received.
     */
    const auto &first = transactions.front();

    this->state = State::SendAddress;
    this->beginTransaction(first, false);

    // wait for the transactions to complete/error out
    ok = xTaskNotifyWaitIndexed(Rtos::TaskNotifyIndex::DriverPrivate, 0,
            Drivers::NotifyBits::I2CMaster, &note, portMAX_DELAY);
    if(!ok) {
        err = Errors::UnspecifiedError;
        goto beach;
    }

    // interpret the result (from interrupts)
    if(!this->completion) { // success
        err = 0;
    }
    // transaction(s) failed
    else {
        err = this->completion;
    }

    // clean up and release the lock
beach:;
    this->waiting = nullptr;

    ok = xSemaphoreGive(this->busLock);
    REQUIRE(ok == pdTRUE, "failed to release I2C bus lock");

    return err;
}



/**
 * @brief Interrupt handler
 *
 * Invoked when the SERCOM irq 0 / "Master on Bus" fires.
 */
void I2C::irqHandler() {
    BaseType_t woken{0};
    const uint8_t irqs{this->regs->INTFLAG.reg};
    const uint32_t status{this->regs->STATUS.reg};
    bool prepareForNext{false}, needsStop{true};

    const auto &txn = this->currentTxns[this->currentTxn];

    // handle according to current state
    switch(this->state) {
        /*
         * Transmitting address. There are one of four possibilities here:
         *
         * 1. Bus error during transmission: INTFLAG.MB and STATUS.BUSERR are set
         *      -> Abort transaction
         * 2. Successful transmission, no ACK received: INTFLAG.MB, STATUS.RXNACK set
         *      -> Clock is stretched (bus frozen)
         *      -> Issue STOP
         *      -> Abort transaction
         * 3. Successful transmission, write: INTFLAG.MB set, STATUS.RXNACK clear
         *      -> Clock is stretched (bus frozen)
         *      -> Transition to writing data (write to DATA.DATA)
         * 4. Successful transmission, read: INTFLAG.SB set, STATUS.RXNACK clear
         *      -> Clock is stretched (bus frozen)
         *      -> Determine if ACK or NACK (last byte of transfer)
         *      -> Read received byte from DATA.DATA
         */
        case State::SendAddress:
            // case 1: bus error
            if((irqs & SERCOM_I2CM_INTFLAG_MB) && (status & SERCOM_I2CM_STATUS_BUSERR)) {
                // abort transaction
                this->irqCompleteTxn(Errors::BusError, &woken);

                // ack irq and update state
                this->state = State::Idle;
                this->regs->INTFLAG.reg = SERCOM_I2CM_INTFLAG_MB;
            }
            // no ACK received
            else if((irqs & SERCOM_I2CM_INTFLAG_MB) && (status & SERCOM_I2CM_STATUS_RXNACK)) {
                // issue STOP and abort transaction
                this->issueStop();
                this->irqCompleteTxn(Errors::NoAck, &woken);

                // update state machine
                this->state = State::Idle;
            }
            // acknowledge received, write transaction
            else if((irqs & SERCOM_I2CM_INTFLAG_MB) && !(status & SERCOM_I2CM_STATUS_RXNACK)) {
                // transmit the first byte
                const auto data = txn.data[this->currentTxnOffset++];
                this->regs->DATA.reg = data;

                // update state machine
                this->state = State::WriteData;
            }
            // acknowledge received, read transaction
            else if((irqs & SERCOM_I2CM_INTFLAG_SB) && !(status & SERCOM_I2CM_STATUS_RXNACK)) {
                /*
                 * If this transaction was a single byte, that byte will have already been received
                 * here, and we'll have replied with a NACK.
                 */
                if(txn.length == 1) {
                    /*
                     * This was the last transaction. Issue a STOP and then notify the task.
                     */
                    if(this->currentTxn == (this->currentTxns.size() - 1)) {
                        this->issueStop();
                        this->irqCompleteTxn(0, &woken);

                        needsStop = false;
                    }
                    /*
                     * Inspect the next transaction. If it's not a continuation, issue a stop;
                     * otherwise a repeated start.
                     */
                    else {
                        const auto &next = this->currentTxns[this->currentTxn + 1];
                        if(next.continuation) {
                            // XXX: this path _might_ be broken
                            this->issueReStart();
                        } else {
                            this->issueStop();
                        }

                        /*
                         * In any case, fetch the next transaction. But ensure that this does not
                         * send another STOP condition.
                         */
                        needsStop = false;
                        prepareForNext = true;
                    }

                    /*
                     * We've taken care of the bus state, and can read out the byte now without
                     * triggering any sort of bus action.
                     */
                    txn.data[this->currentTxnOffset++] = this->regs->DATA.reg & 0xff;
                }
                /*
                 * The transaction has at least another byte after this one; so set the acknowledge
                 * action for that byte (if the transfer is 2 bytes, NACK that second byte) and
                 * copy it into the buffer.
                 *
                 * Termination logic is in the ReadData state.
                 */
                else {
                    // if the next byte is the last, NACK it
                    if(txn.length == 2) {
                        this->regs->CTRLB.reg = SERCOM_I2CM_CTRLB_SMEN | SERCOM_I2CM_CTRLB_ACKACT;
                    }
                    // otherwise, ACK it
                    else {
                        this->regs->CTRLB.reg = SERCOM_I2CM_CTRLB_SMEN;
                    }

                    // read out the byte
                    txn.data[this->currentTxnOffset++] = this->regs->DATA.reg & 0xff;
                }

                // update state machine
                this->state = State::ReadData;
            }
            // other cases, should _not_ happen
            else {
                Logger::Panic("SERCOM I2C irq error: state %u (irq %02x status %08x)",
                        static_cast<unsigned int>(this->state), irqs, status);
            }
            break;

        /*
         * Transmitting data
         *
         * 1. Byte sent, acknowledged by slave: INTFLAG.MB set, STATUS.RXNACK clear
         *      -> Clock is stretched (bus frozen)
         *      -> If not end: write next data byte
         *      -> If end: peek at next transaction, issue STOP if continuation bit clear
         * 2. Byte sent, NACK received: INTFLAG.MB set, STATUS.RXNACK set
         *      -> Clock is stretched (bus frozen)
         *      -> Issue STOP
         *      -> Abort transaction
         */
        case State::WriteData: {
            // byte sent and acknowledged
            if((irqs & SERCOM_I2CM_INTFLAG_MB) && !(status & SERCOM_I2CM_STATUS_RXNACK)) {
                // if this was the last byte, not much to do; prepare for the next transaction
                if(this->currentTxnOffset == txn.length) {
                    prepareForNext = true;
                }
                // otherwise, write the next byte
                else {
                    const auto data = txn.data[this->currentTxnOffset++];
                    this->regs->DATA.reg = data;
                }
            }
            // byte sent, NACK received
            else if((irqs & SERCOM_I2CM_INTFLAG_MB) && (status & SERCOM_I2CM_STATUS_RXNACK)) {
                this->issueStop();
                this->irqCompleteTxn(Errors::UnexpectedNAck, &woken);

                needsStop = false;
            }
            // other error
            else {
                this->issueStop();
                this->irqCompleteTxn(Errors::TransmissionError, &woken);

                needsStop = false;

                Logger::Panic("SERCOM I2C irq error: state %u (irq %02x status %08x)",
                        static_cast<unsigned int>(this->state), irqs, status);
            }

            break;
        }

        /*
         * Receiving data (second or later byte)
         *
         * 1. Byte received: INTFLAG.SB set
         *      -> If last byte: Set CTRLB.ACKACT to generate NACK
         *      -> If not last byte: Clear CTRLB.ACKACT to generate ACK
         *      -> Copy byte into receive buffer
         */
        case State::ReadData:
            if((irqs & SERCOM_I2CM_INTFLAG_SB)) {
                // if the next byte is the last, NACK it
                if(txn.length == (this->currentTxnOffset + 2)) {
                    this->regs->CTRLB.reg = SERCOM_I2CM_CTRLB_SMEN | SERCOM_I2CM_CTRLB_ACKACT;
                }
                // otherwise, ACK the next byte
                else {
                    this->regs->CTRLB.reg = SERCOM_I2CM_CTRLB_SMEN;
                }

                // read out the byte and store in buffer
                txn.data[this->currentTxnOffset++] = this->regs->DATA.reg & 0xff;

                // did we receive the last byte?
                if(this->currentTxnOffset == txn.length) {
                    prepareForNext = true;
                }
            }
            // unknown error
            else {
                this->issueStop();
                this->irqCompleteTxn(Errors::ReceptionError, &woken);

                Logger::Panic("SERCOM I2C irq error: state %u (irq %02x status %08x)",
                        static_cast<unsigned int>(this->state), irqs, status);
            }
            break;

        /*
         * Spurious interrupt
         *
         * This should really never happen
         */
        case State::Idle:
            Logger::Panic("Invalid SERCOM I2C state: %u (irq %02x status %08x)",
                    static_cast<unsigned int>(this->state), irqs, status);
            break;
    }

    /*
     * If needed, set up for the next transaction. This will write the ADDR.ADDR field with the
     * correct read/write bit.
     *
     * Note that when we get here, the bus may still be frozen. We'll need to issue a STOP or a
     * repeated START condition to make the bus "go" again, depending on whether the next
     * transaction is a continuation of the previous one or not.
     *
     * Since we need to send a command (for START/STOP) we _cannot_ acknowledge SB or MB interrupt
     * flags before reaching here; that's done automatically by writing the command or address.
     */
    if(prepareForNext) {
        /*
         * In case this was the last transaction we need to deal with, issue a STOP condition and
         * notify the task.
         */
        if(this->currentTxn == (this->currentTxns.size() - 1)) {
            if(!needsStop) this->issueStop();
            this->irqCompleteTxn(0, &woken);

            this->state = State::Idle;
        }
        /*
         * Otherwise, read the information for the next transaction, and start it.
         */
        else {
            // reset state machine
            this->state = State::SendAddress;
            this->currentTxnOffset = 0;

            // begin the next transaction
            this->beginTransaction(this->currentTxns[++this->currentTxn], needsStop);
        }
    }

    portYIELD_FROM_ISR(woken);
}

/**
 * @brief Terminate the currently executing transaction with a status code
 *
 * @param status Error/status code
 * @param woken Set when a higher priority task became runnable
 */
void I2C::irqCompleteTxn(const int status, BaseType_t *woken) {
    // update status
    this->completion = status;
    __DSB();

    // notify
    xTaskNotifyIndexedFromISR(this->waiting, Rtos::TaskNotifyIndex::DriverPrivate,
            Drivers::NotifyBits::I2CMaster, eSetBits, woken);
}

/**
 * @brief Begin a new transaction
 *
 * Issue either a STOP or repeated START, then the address of the next transaction, which begins
 * the address phase of the transaction.
 *
 * @param needsStop Whether we need to issue a STOP or repeated START
 */
void I2C::beginTransaction(const Transaction &txn, const bool needsStop) {
    // issue either a stop or repeated start
    if(needsStop) {
        if(txn.continuation) {
            this->issueReStart();
        } else {
            this->issueStop();
        }
    }

    // set the acknowledge action if read
    if(txn.read && txn.length == 1) {
        this->regs->CTRLB.reg = SERCOM_I2CM_CTRLB_SMEN | SERCOM_I2CM_CTRLB_ACKACT;
    } else {
        this->regs->CTRLB.reg = SERCOM_I2CM_CTRLB_SMEN;
    }

    // send the address
    this->regs->ADDR.reg = ((txn.address & 0x7f) << 1) | (txn.read ? 0b1 : 0b0);
    __DSB();

    this->waitSysOpSync();
}



/**
 * @brief Configure the SERCOM I²C registers based on the provided configuration.
 *
 * @param regs Registers to receive the configuration
 * @param conf Peripheral configuration
 *
 * @remark The peripheral should be disabled when invoking this; it's best to perform a reset
 *         before.
 */
void I2C::ApplyConfiguration(const SercomBase::Unit unit, ::SercomI2cm *regs, const Config &conf) {
    /*
     * Calculate the appropriate baud rate and set it
     */
    UpdateFreq(unit, regs, conf.frequency);

    /*
     * CTRLA: Control A
     *
     * Copy from configuration: SCL low timeout
     *
     * Fixed: I²C master mode, clock stretch after ACK, transfer speed (based on frequency)
     */
    uint32_t ctrla{0};

    if(conf.sclLowTimeout) {
        ctrla |= SERCOM_I2CM_CTRLA_LOWTOUTEN;
    }

    if(conf.frequency <= 400'000) { // Sm and Fm
        ctrla |= SERCOM_I2CM_CTRLA_SPEED(0x0);
    }
    else if(conf.frequency <= 1'000'000) { // Fm+
        ctrla |= SERCOM_I2CM_CTRLA_SPEED(0x1);
    }
    else if(conf.frequency <= 3'400'000) { // Hs
        ctrla |= SERCOM_I2CM_CTRLA_SPEED(0x2);
    }

    ctrla |= SERCOM_I2CM_CTRLA_SCLSM;

    ctrla |= SERCOM_SPI_CTRLA_MODE(static_cast<uint8_t>(SercomBase::Mode::I2CMaster));

    Logger::Debug("SERCOM%u %s %s: $%08x", static_cast<unsigned int>(unit), "I2C", "CTRLA", ctrla);
    regs->CTRLA.reg = ctrla & SERCOM_I2CM_CTRLA_MASK;

    /*
     * CTRLB: Control B
     *
     * Enable smart mode (acknowledge sent when DATA.DATA is read)
     */
    regs->CTRLB.reg = SERCOM_I2CM_CTRLB_SMEN;

    /**
     * CTRLC: Control C
     *
     * Data transfers are 8-bit
     */
    //regs->CTRLC.reg = SERCOM_I2CM_CTRLC_DATA32B;
    regs->CTRLC.reg = 0;
}

/**
 * @brief Sets the I²C clock frequency
 *
 * @param unit SERCOM unit
 * @param regs Registers to receive the configuration
 * @param frequency Desired frequency, in Hz
 *
 * @remark If the exact frequency cannot be achieved, the calculation will round down.
 */
void I2C::UpdateFreq(const SercomBase::Unit unit, ::SercomI2cm *regs,
        const uint32_t frequency) {
    const auto core = SercomBase::CoreClockFor(unit);
    REQUIRE(core, "SERCOM%u: core clock unknown", static_cast<unsigned int>(unit));

    const auto baud = (core / (2 * frequency)) - 1;
    const auto actual = core / (2 * (baud + 1));

    REQUIRE(baud <= 0xFF, "I2C baud rate out of range (%u Hz = $%08x)", frequency, baud);

    Logger::Debug("SERCOM%u I2C freq: request %u Hz, got %u Hz", static_cast<unsigned int>(unit),
            frequency, actual);

    regs->BAUD.reg = baud;
}
