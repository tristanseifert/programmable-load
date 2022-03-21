#ifndef DRIVERS_I2CDEVICE_PCA9543A_H
#define DRIVERS_I2CDEVICE_PCA9543A_H

#include "Rtos/Rtos.h"

#include <etl/optional.h>

namespace Drivers {
class I2CBus;
}

/// I²C device drivers
namespace Drivers::I2CDevice {
/**
 * @brief Driver for the PCA9543A 2-channel I²C switch
 *
 * The driver exposes two downstream pseudo-bus instances. Each of these bus instances will cause
 * the mux to automagically switch to the appropriate downstream bus, if necessary, before the
 * transaction completes. If the bus is already set to the correct option, no switch is required
 * and the transactions begin immediately.
 *
 * Additionally, the implementation guarantees that all transactions on one downstream bus will
 * complete without being interrupted by ones sent to the other bus.
 */
class PCA9543A {
    public:
        PCA9543A(const uint8_t address, Drivers::I2CBus *parent);
        ~PCA9543A();

        int readIrqState(bool &irq0, bool &irq1);

        int activateBus(const uint8_t bus);
        int deactivateBus();

        /**
         * @brief Get downstream bus 0
         */
        constexpr inline Drivers::I2CBus *getDownstream0() {
            return nullptr;
        }
        /**
         * @brief Get downstream bus 1
         */
        constexpr inline Drivers::I2CBus *getDownstream1() {
            return nullptr;
        }

    private:
        int sendPacket(const uint8_t data);
        int readStatus(uint8_t &outStatus);

    private:
        /// Parent bus
        Drivers::I2CBus *bus;
        /// Device address
        uint8_t address;
        /// Currently active bus
        etl::optional<uint8_t> activeBus;

        /**
         * @brief Bus lock
         *
         * Any time a transaction takes place against either of the downstream busses, or the mux
         * itself, we need to take this lock to ensure we don't cause some sort of jankiness.
         *
         * This is a recursive mutex, so that if we need to switch the bus as we start performing
         * a transaction on the downstream bus, that's acceptable by just taking the lock once
         * more.
         */
        SemaphoreHandle_t busLock;
        /// storage for the bus lock
        StaticSemaphore_t busLockStorage;
};
}

#endif
