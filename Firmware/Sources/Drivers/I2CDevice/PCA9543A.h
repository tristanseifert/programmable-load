#ifndef DRIVERS_I2CDEVICE_PCA9543A_H
#define DRIVERS_I2CDEVICE_PCA9543A_H

#include "Drivers/I2CBus.h"
#include "Rtos/Rtos.h"

#include <etl/array.h>
#include <etl/optional.h>

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
 *
 * @remark Though the hardware supports multiple simultaneous channels enabled, this driver will
 *         enforce that only a single channel is active at a time.
 */
class PCA9543A {
    public:
        /**
         * @brief An I²C downstream bus
         *
         * This is a thin wrapper around the upstream bus that will switch to the appropriate
         * downstream bus, if needed, before the transaction is performed.
         *
         * You can use this from application code the same way as other I²C busses.
         */
        class DownstreamBus: public Drivers::I2CBus {
            friend class PCA9543A;

            private:
                /**
                 * @brief Initialize a downstream bus
                 *
                 * @param parent Multiplexer instance that owns this bus
                 * @param channel Channel index of this downstream bus
                 *
                 * @remark This is automatically done when the mux is initialized. You should not
                 *         try to manually create one.
                 */
                DownstreamBus(PCA9543A *parent, const uint8_t channel) : parent(parent),
                    channel(channel) {}

            public:
                int perform(etl::span<const Transaction> transactions) override;

            private:
                /**
                 * @brief Bus switch that owns this downstream bus
                 *
                 * This is the bus switch on which this downstream bus is located. The upsteram
                 * bus of the mux is used to perform all transactions; this mux is also used to
                 * handle the locking.
                 */
                PCA9543A *parent;

                /**
                 * @brief Bus index
                 *
                 * Index, on the parent mux, of this channel. Used to activate the bus if needed.
                 */
                uint8_t channel;
        };

    public:
        PCA9543A(const uint8_t address, Drivers::I2CBus *parent);
        ~PCA9543A();

        int readIrqState(bool &irq0, bool &irq1);

        /**
         * @brief Determine the currently active bus
         *
         * @return Active bus number, or `etl::nullopt` if none is active
         */
        inline etl::optional<uint8_t> getActiveBus() const {
            return this->activeBus;
        }

        int activateBus(const uint8_t bus);
        int deactivateBus();

        /**
         * @brief Get downstream bus 0
         */
        constexpr inline Drivers::I2CBus *getDownstream0() {
            return &this->busses[0];
        }
        /**
         * @brief Get downstream bus 1
         */
        constexpr inline Drivers::I2CBus *getDownstream1() {
            return &this->busses[1];
        }

    protected:
        int sendPacket(const uint8_t data);
        int readStatus(uint8_t &outStatus);

    protected:
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

        /**
         * @brief Downstream busses
         *
         * A list of downstream busses, which is initialized when we construct the mux. Each of
         * these busses corresponds to one downstream bus, and automagically handles switching the
         * appropriate channel.
         */
        etl::array<DownstreamBus, 2> busses;
};
}

#endif
