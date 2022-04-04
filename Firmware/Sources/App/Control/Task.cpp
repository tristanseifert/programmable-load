#include "Task.h"
#include "Hardware.h"

#include "Drivers/I2C.h"
#include "Drivers/I2CDevice/AT24CS32.h"

#include "Log/Logger.h"
#include "Rtos/Rtos.h"
#include "Util/InventoryRom.h"

#include <string.h>
#include <etl/array.h>

using namespace App::Control;

StaticTask_t Task::gTcb;
StackType_t Task::gStack[kStackSize];

Task *Task::gShared{nullptr};

/**
 * @brief Start the control task
 *
 * This initializes the shared control task instance.
 */
void App::Control::Start() {
    static uint8_t gTaskBuf[sizeof(Task)] __attribute__((aligned(alignof(Task))));
    auto ptr = reinterpret_cast<Task *>(gTaskBuf);

    Task::gShared = new (ptr) Task();
}

/**
 * @brief Initialize the control task
 */
Task::Task() {
    this->task = xTaskCreateStatic([](void *ctx) {
        reinterpret_cast<Task *>(ctx)->main();
        Logger::Panic("what the fuck");
    }, kName.data(), kStackSize, this, kPriority, gStack, &gTcb);
}

/**
 * @brief Control main loop
 *
 * Entry point for the control loop task; this monitors the current/voltage sense ADCs, feeds
 * their inputs into the control algorithm, then uses that to drive the current control DACs.
 *
 * The expansion bus is scanned to determine what driver board is connected.
 */
void Task::main() {
    BaseType_t ok;
    uint32_t note;

    /*
     * Initialize the driver board. This will probe the board's identification EEPROM, and attempt
     * to figure out what hardware is connected. We'll then go and initialize the appropriate
     * controller driver instance, which in turn initializes the hardware on the driver.
     */
    Logger::Trace("control: %s", "identify hardware");
    this->identifyDriver();

    /*
     * Start handling messages
     */
    Logger::Trace("control: %s", "start message loop");

    while(1) {
        ok = xTaskNotifyWaitIndexed(kNotificationIndex, 0, TaskNotifyBits::All, &note,
                portMAX_DELAY);
        REQUIRE(ok == pdTRUE, "%s failed: %d", "xTaskNotifyWaitIndexed", ok);

        // TODO: handle
        Logger::Warning("control notify: $%08x", note);
    }
}

/**
 * @brief Read the driver board identification ROM
 *
 * This will read the serial number + first 16 bytes of the EEPROM to determine if there is a full
 * identification page available.
 */
void Task::identifyDriver() {
    int err;
    etl::array<uint8_t, 16> serial;
    etl::array<uint8_t, 16> header;

    /*
     * Read the serial first. If this fails, we know that there's no device connected, since the
     * EEPROM always responds.
     */
    Drivers::I2CDevice::AT24CS32 idprom(Hw::gBus);

    err = idprom.readSerial(serial);
    REQUIRE(!err, "failed to read controller %s: %d", "serial", err);

    Logger::Notice("controller serial: %02x%02x%02x%02x%02x%02x%02x%02x"
            "%02x%02x%02x%02x%02x%02x%02x%02x", serial[0], serial[1], serial[2], serial[3],
            serial[4], serial[5], serial[6], serial[7], serial[8], serial[9], serial[10],
            serial[11], serial[12], serial[13], serial[14], serial[15]);

    /*
     * Now read the identification data out of the ROM. This consists first of a fixed 16-byte
     * header that we verify for correctness, then one or more "atoms" that actually contain
     * payload data.
     *
     * For this application, we really just care about the "driver identifier" which is an UUID
     * matching to one of the driver classes we have.
     */
    err = idprom.readData(0x000, header);
    REQUIRE(!err, "failed to read controller %s: %d", "prom header", err);

    // TODO: get actual start addr from header
    err = Util::InventoryRom::GetAtoms(0x010,
            // TODO: take length into account
            [](auto addr, auto len, auto buf, auto ctx) -> int {
        return reinterpret_cast<Drivers::I2CDevice::AT24CS32 *>(ctx)->readData(addr, buf);
    }, &idprom,
    /*
     * We only want to read the driver UUID and hardware revision atoms.
     */
    [](auto header, auto ctx, auto readBuf) -> bool {
        static etl::array<uint8_t, 16> gUuidBuf;
        static etl::array<uint8_t, 2> gRevBuf;

        switch(header.type) {
            case Util::InventoryRom::AtomType::HwRevision:
                readBuf = gRevBuf;
                break;
            case Util::InventoryRom::AtomType::DriverId:
                readBuf = gUuidBuf;
                break;

            default:
                break;
        }
        return true;
    }, this,
    /*
     * Deal with the driver uuid or hw revision values.
     */
    [](auto header, auto buffer, auto ctx) {
        auto inst = reinterpret_cast<Task *>(ctx);

        switch(header.type) {
            /*
             * Hardware revision is represented as a big endian 16-bit integer.
             */
            case Util::InventoryRom::AtomType::HwRevision:
                memcpy(&inst->driverRev, buffer.data(), 2);
                break;
            /*
             * Driver ID is encoded as a 16 byte binary representation of an UUID.
             */
            case Util::InventoryRom::AtomType::DriverId:
                inst->driverId = Util::Uuid(buffer);
                break;

            default:
                break;
        }
    }, this);

    REQUIRE(err >= 0, "failed to read controller %s: %d", "prom atoms", err);

    // log info about it
    etl::array<char, 0x26> uuidStr;
    this->driverId.format(uuidStr);

    Logger::Notice("Load pcb: rev %u (driver %s)", this->driverRev, uuidStr);
}
