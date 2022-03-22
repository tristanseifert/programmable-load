#include "PCA9543A.h"

#include "Drivers/I2CBus.h"
#include "Log/Logger.h"

#include <etl/array.h>

using namespace Drivers::I2CDevice;

/**
 * @brief Initialize the PCA9543A bus switch
 *
 * @remark This assumes the switch has been just initialized the same as a power on reset, or by
 *         toggling /RESET.
 */
PCA9543A::PCA9543A(const uint8_t address, Drivers::I2CBus *parent) : bus(parent), address(address),
    busses({DownstreamBus(this, 0), DownstreamBus(this, 1)}) {
    // initialize locking
    this->busLock = xSemaphoreCreateRecursiveMutexStatic(&this->busLockStorage);

    // read currently active bus
    int err;
    uint8_t status{0};

    err = this->readStatus(status);
    REQUIRE(!err, "%s: failed to read status: %d", "PCA9543A", err);

    if(status & (1 << 0)) {
        this->activeBus = 0;
    } else if(status & (1 << 1)) {
        this->activeBus = 1;
    }
}

/**
 * @brief Deinitialize the bus switch
 *
 * This will deactivate both busses and deallocate resources.
 */
PCA9543A::~PCA9543A() {
    this->deactivateBus();

    // clean up resources
    vSemaphoreDelete(this->busLock);
}

/**
 * @brief Get interrupt state of downstream busses
 *
 * Query the mux to figure out which of the downstream busses have their interrupt lines asserted.
 *
 * @param irq0 Downstream channel 0 is asserting interrupt
 * @param irq1 Downstream channel 1 is asserting interrupt
 */
int PCA9543A::readIrqState(bool &irq0, bool &irq1) {
    int err{0};
    uint8_t status;

    // read status register
    err = this->readStatus(status);
    if(!err) {
        irq0 = (status & (1 << 4));
        irq1 = (status & (1 << 5));
    }

    return err;
}

/**
 * @brief Activate a particular bus on the mux.
 *
 * @param activate A bus number ([0,1]) to switch to
 */
int PCA9543A::activateBus(const uint8_t activate) {
    REQUIRE(activate < 2, "%s: invalid bus %u", "PCA9543A", activate);

    const auto err = this->sendPacket((1 << activate));

    if(!err) {
        this->activeBus = activate;
    }

    return err;
}

/**
 * @brief Deactivate all downstream busses.
 */
int PCA9543A::deactivateBus() {
    const auto err = this->sendPacket(0x00);

    if(!err) {
        this->activeBus = etl::nullopt;
    }

    return err;
}

/**
 * @brief Write a byte to the device control register
 *
 * @param data Data to write to the control register
 *
 * @return 0 on success
 */
int PCA9543A::sendPacket(const uint8_t data) {
    BaseType_t ok;
    int err;
    etl::array<uint8_t, 1> txBuf{{data}};

    // acquire bus lock
    ok = xSemaphoreTakeRecursive(this->busLock, portMAX_DELAY);
    if(ok != pdTRUE) {
        return -1;
    }

    // write data
    etl::array<I2CBus::Transaction, 1> txns{{
        // first transaction: write 1 byte to device
        {
            .address = this->address,
            .read = 0,
            .length = 1,
            .data = txBuf
        },
    }};
    err = this->bus->perform(txns);

    // release bus lock
    xSemaphoreGiveRecursive(this->busLock);
    return err;
}

/**
 * @brief Read the status/control register of the device.
 *
 * @param outStatus Variable to receive the register value
 *
 * @return 0 on success
 */
int PCA9543A::readStatus(uint8_t &outStatus) {
    BaseType_t ok;
    int err{0};
    etl::array<uint8_t, 1> rxBuf;

    // acquire bus lock
    ok = xSemaphoreTakeRecursive(this->busLock, portMAX_DELAY);
    if(ok != pdTRUE) {
        return -1;
    }

    // perform the read
    etl::array<I2CBus::Transaction, 1> txns{{
        // first transaction: read 1 byte from device
        {
            .address = this->address,
            .read = 1,
            .length = 1,
            .data = rxBuf
        },
    }};

    err = this->bus->perform(txns);

    // fish out value if success
    if(!err) {
        outStatus = rxBuf[0];
    }

    // release bus lock
    xSemaphoreGiveRecursive(this->busLock);
    return err;
}



/**
 * @brief Perform a bus transaction on a downstream bus
 *
 * This will ensure that the mux has activated the appropriate channel, then forwards the actual
 * transaction to the parent bus we're connected to. The bus lock on the mux is held for the entire
 * duration of the call.
 */
int PCA9543A::DownstreamBus::perform(etl::span<const Transaction> transactions) {
    BaseType_t ok;
    int err;

    // acquire bus lock
    ok = xSemaphoreTakeRecursive(this->parent->busLock, portMAX_DELAY);
    if(ok != pdTRUE) {
        return -1;
    }

    // switch channel, if needed
    if(this->parent->activeBus != this->channel) {
        err = this->parent->activateBus(this->channel);
        if(err) {
            goto beach;
        }
    }

    // perform txn
    err = this->parent->bus->perform(transactions);

    // release bus lock
beach:;
    xSemaphoreGiveRecursive(this->parent->busLock);
    return err;
}

