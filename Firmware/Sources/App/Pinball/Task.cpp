#include "Task.h"
#include "Hardware.h"
#include "FrontIo/Display.h"
#include "FrontIo/HmiDriver.h"

#include "Drivers/ExternalIrq.h"
#include "Drivers/Gpio.h"
#include "Drivers/Spi.h"
#include "Drivers/I2CDevice/AT24CS32.h"
#include "Log/Logger.h"
#include "Rtos/Rtos.h"
#include "Util/Base32.h"
#include "Util/InventoryRom.h"

#include <etl/array.h>

#include <vendor/sam.h>

using namespace App::Pinball;

StaticTask_t Task::gTcb;
StackType_t Task::gStack[kStackSize];

Task *Task::gShared{nullptr};

/**
 * @brief Start the pinball task
 *
 * This initializes the shared pinball task instance.
 */
void App::Pinball::Start() {
    static uint8_t gTaskBuf[sizeof(Task)] __attribute__((aligned(alignof(Task))));
    auto ptr = reinterpret_cast<Task *>(gTaskBuf);

    Task::gShared = new (ptr) Task();
}

/**
 * @brief Initialize the UI task
 */
Task::Task() {
    this->task = xTaskCreateStatic([](void *ctx) {
        reinterpret_cast<Task *>(ctx)->main();
        Logger::Panic("what the fuck");
    }, kName.data(), kStackSize, this, kPriority, gStack, &gTcb);
}

/**
 * @brief Pinball main loop
 *
 * Responds to user interface events (such as button presses, encoder rotations, etc.) and then
 * updates the interface (display, indicators) appropriately.
 */
void Task::main() {
    BaseType_t ok;
    uint32_t note;

    /*
     * Initialize front panel hardware
     *
     * Reset all hardware for front IO, then begin by initializing the display. After this is
     * complete, read the ID EEPROM on the I2C bus to determine what devices/layout is available on
     * the front panel, and initialize those devices.
     */
    Logger::Trace("pinball: %s", "reset hw");
    Hw::ResetFrontPanel();

    // initialize display
    Logger::Trace("pinball: %s", "init display");
    Display::Init();

    // discover front panel hardware, and initialize itz
    Logger::Trace("pinball: %s", "init front panel");
    this->initEncoder();
    this->detectFrontPanel();

    /*
     * Start handling messages
     *
     * As with the app main task, we chill waiting on the task notification value. This will be
     * set to one or more bits, which in turn indicate what we need to do. If additional data needs
     * to be passed, it'll be in the appropriate queues.
     */
    Logger::Trace("pinball: %s", "start message loop");

    while(1) {
        ok = xTaskNotifyWaitIndexed(kNotificationIndex, 0, TaskNotifyBits::All, &note,
                portMAX_DELAY);
        REQUIRE(ok == pdTRUE, "%s failed: %d", "xTaskNotifyWaitIndexed", ok);

        Logger::Warning("pinball notify: $%08x", note);
        if(note & TaskNotifyBits::FrontIrq) {
            this->frontDriver->handleIrq();
        }
        if(note & TaskNotifyBits::EncoderChanged) {
            this->updateEncoder();
        }
    }
}

/**
 * @brief Initialize encoder state machine
 *
 * Sets up the state of the rotary encoder reading state machine. We'll sample the input pins here
 * to get the baseline value.
 */
void Task::initEncoder() {

}

/**
 * @brief Read the state of the encoder pins and update UI
 *
 * Advances the state of the encoder state machine to determine whether we should increment or
 * decrement the current value.
 */
void Task::updateEncoder() {
}



/**
 * @brief Detect front panel hardware
 *
 * Tries to find an AT24CS32 EEPROM on the front panel bus. This in turn will contain a small
 * struct that identifies what type of hardware we have installed.
 */
void Task::detectFrontPanel() {
    int err;
    etl::array<uint8_t, 16> serial;
    etl::array<char, 28> serialBase32;

    // try to read the serial number EEPROM
    Drivers::I2CDevice::AT24CS32 idprom(Hw::gFrontI2C);

    err = idprom.readSerial(serial);
    if(err) {
        Logger::Warning("failed to ID front I/O: %d", err);
        return;
    }

    Util::Base32::Encode(serial, serialBase32);
    Logger::Notice("front IO S/N: %s", serialBase32.data());

    // parse hw rev, driver id off the ROM
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
                inst->frontRev = __builtin_bswap16(temp);
                break;
            /*
             * Driver ID is encoded as a 16 byte binary representation of an UUID.
             */
            case Util::InventoryRom::AtomType::DriverId:
                inst->frontDriverId = Util::Uuid(buffer);
                break;

            default:
                break;
        }
    }, this);

    REQUIRE(err >= 0, "failed to ID front panel: %d", err);

    etl::array<char, 0x26> uuidStr;
    this->frontDriverId.format(uuidStr);

    Logger::Notice("front I/O: rev %u (driver %s)", this->frontRev, uuidStr.data());

    /*
     * We only support one type of front panel right now, so ensure that the driver ID in the
     * IDPROM matches that, then instantiate it.
     */
    REQUIRE(this->frontDriverId == HmiDriver::kDriverId, "unknown front I/O driver: %s",
            uuidStr.data());

    static uint8_t gHmiDriverBuf[sizeof(HmiDriver)] __attribute__((aligned(alignof(HmiDriver))));
    auto ptr = reinterpret_cast<HmiDriver *>(gHmiDriverBuf);

    this->frontDriver = new (ptr) HmiDriver(Hw::gFrontI2C, idprom);
}
