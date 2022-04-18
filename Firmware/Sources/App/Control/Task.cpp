#include "Task.h"
#include "Hardware.h"
#include "LoadDriver.h"
#include "DumbLoadDriver.h"

#include "App/Main/Task.h"
#include "Drivers/I2C.h"
#include "Drivers/I2CDevice/AT24CS32.h"

#include "Log/Logger.h"
#include "Rtos/Rtos.h"
#include "Util/Base32.h"
#include "Util/InventoryRom.h"

#include <string.h>
#include <etl/array.h>

using namespace App::Control;

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
    // create the task
    this->task = xTaskCreateStatic([](void *ctx) {
        reinterpret_cast<Task *>(ctx)->main();
        Logger::Panic("what the fuck");
    }, kName.data(), kStackSize, this, kPriority, this->stack, &this->tcb);

    // also, create the timer (to force sampling of data)
    this->sampleTimer = xTimerCreateStatic("Control sample timer",
        // automagically reload
        pdMS_TO_TICKS(kMeasureInterval), pdTRUE,
        this, [](auto timer) {
            Task::NotifyTask(Task::TaskNotifyBits::SampleData);
        }, &this->sampleTimerBuf);
    REQUIRE(this->sampleTimer, "control: %s", "failed to allocate timer");
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
    int err;

    /*
     * Initialize the driver board. This will probe the board's identification EEPROM, and attempt
     * to figure out what hardware is connected. We'll then go and initialize the appropriate
     * controller driver instance, which in turn initializes the hardware on the driver.
     */
    Logger::Trace("control: %s", "identify hardware");
    Hw::PulseReset();

    this->identifyDriver();

    /*
     * Start handling messages
     */
    Logger::Trace("control: %s", "start message loop");

    xTimerStart(this->sampleTimer, portMAX_DELAY);

    while(1) {
        ok = xTaskNotifyWaitIndexed(kNotificationIndex, 0, TaskNotifyBits::All, &note,
                portMAX_DELAY);
        REQUIRE(ok == pdTRUE, "%s failed: %d", "xTaskNotifyWaitIndexed", ok);

        // handle interrupt and triggers
        if(note & TaskNotifyBits::IrqAsserted) {
            this->driver->handleIrq();
        }

        // sample sensors
        if(note & TaskNotifyBits::SampleData) {
            this->readSensors();
        }

        // update sense input relay
        if(note & TaskNotifyBits::UpdateSenseRelay) {
            err = this->driver->setExternalVSense(this->isUsingExternalSense);
            REQUIRE(!err, "control: %s (%d)", "failed to change sense relay", err);
        }

        // check in with watchdog
        App::Main::Task::CheckIn(App::Main::WatchdogCheckin::Control);
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
    etl::array<char, 28> serialBase32;

    /*
     * Read the serial first. If this fails, we know that there's no device connected, since the
     * EEPROM always responds.
     */
    Drivers::I2CDevice::AT24CS32 idprom(Hw::gBus);

    err = idprom.readSerial(serial);
    REQUIRE(!err, "failed to read driver pcb %s: %d", "serial", err);

    Util::Base32::Encode(serial, serialBase32);
    Logger::Notice("driver pcb serial: %s", serialBase32.data());

    /*
     * Now read the identification data out of the ROM. This consists first of a fixed 16-byte
     * header that we verify for correctness, then one or more "atoms" that actually contain
     * payload data.
     *
     * For this application, we really just care about the "driver identifier" which is an UUID
     * matching to one of the driver classes we have.
     */
    err = Util::InventoryRom::GetAtoms(
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
                uint16_t temp;
                memcpy(&temp, buffer.data(), 2);
                inst->pcbRev = __builtin_bswap16(temp);
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

    REQUIRE(err >= 0, "failed to read driver pcb %s: %d", "prom atoms", err);

    // log info about it
    etl::array<char, 0x26> uuidStr;
    this->driverId.format(uuidStr);

    Logger::Notice("Driver pcb: rev %u (driver %s)", this->pcbRev, uuidStr.data());

    /*
     * Currently there is only support for the "dumb" load boards, so ensure we instantiate the
     * right driver for that UUID.
     */
    REQUIRE(this->driverId == DumbLoadDriver::kDriverId, "unknown load pcb driver: %s",
            uuidStr.data());

    static uint8_t gDriverBuf[sizeof(DumbLoadDriver)]
        __attribute__((aligned(alignof(DumbLoadDriver))));
    auto ptr = reinterpret_cast<DumbLoadDriver *>(gDriverBuf);

    this->driver = new (ptr) DumbLoadDriver(Hw::gBus, idprom);
}



/**
 * @brief Read analog board sensors
 *
 * This updates the cached current and voltage readings.
 */
void Task::readSensors() {
    int err;

    // read input voltage
    err = this->driver->readInputVoltage(this->inputVoltage);
    REQUIRE(!err, "control: %s (%d)", "failed to read input voltage", err);
}
