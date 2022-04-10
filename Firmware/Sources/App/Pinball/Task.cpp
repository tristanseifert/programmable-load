#include "Task.h"
#include "Hardware.h"
#include "FrontIo/Display.h"
#include "FrontIo/HmiDriver.h"

#include "Drivers/ExternalIrq.h"
#include "Drivers/Gpio.h"
#include "Drivers/Spi.h"
#include "Drivers/I2CDevice/AT24CS32.h"
#include "Gui/ScreenManager.h"
#include "Log/Logger.h"
#include "Rtos/Rtos.h"
#include "Util/Base32.h"
#include "Util/InventoryRom.h"

#include <etl/array.h>

#include <BuildInfo.h>
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
    int err;
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

    // discover front panel hardware, and initialize it
    Logger::Trace("pinball: %s", "init front panel");
    this->detectFrontPanel();

    // with the display and front panel IO set up, initialize the GUI
    Gui::ScreenManager::Init();
    this->showVersionScreen();

    /*
     * Start handling messages
     *
     * As with the app main task, we chill waiting on the task notification value. This will be
     * set to one or more bits, which in turn indicate what we need to do. If additional data needs
     * to be passed, it'll be in the appropriate queues.
     */
    Logger::Trace("pinball: %s", "start message loop");

    while(1) {
        bool uiDirty{false};

        ok = xTaskNotifyWaitIndexed(kNotificationIndex, 0, TaskNotifyBits::All, &note,
                portMAX_DELAY);
        REQUIRE(ok == pdTRUE, "%s failed: %d", "xTaskNotifyWaitIndexed", ok);

        if(note & TaskNotifyBits::PowerPressed) {
            Logger::Warning("!!! Power button changed");
        }

        /*
         * Handle front panel interactions: IRQs from the HMI board are forwarded to the driver,
         * which will call into the GUI code with updated button states. If the encoder state was
         * changed (handled in Hw class state machine) we'll read it out and forward it to the
         * GUI task as well.
         */
        if(note & TaskNotifyBits::FrontIrq) {
            this->frontDriver->handleIrq();
        }
        if(note & TaskNotifyBits::EncoderChanged) {
            const auto delta = Hw::ReadEncoderDelta();
            Logger::Trace("Encoder delta: %d", delta);
        }

        /*
         * We can select some main screen modes here to replace the entirety of what's on screen
         * at a time. This is only used for the home screen now.
         */
        if(note & TaskNotifyBits::ShowHomeScreen) {
            // cancel timer (in case a button press got us here)
            xTimerStop(this->versionDismissTimer, 0);

            // TODO: present it
            Logger::Warning("XXX: this is where we'd present the home screen");
            uiDirty = true;
        }

        /*
         * Redraw the user interface, when it's been explicitly requested.
         */
        if(note & TaskNotifyBits::RedrawUI || uiDirty) {
            Gui::ScreenManager::Draw();

            err = Display::Transfer();
            REQUIRE(!err, "pinball: %s (%d)", "failed to transfer display buffer", err);
        }
    }
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



#include "Gui/Components/StaticLabel.h"
#include "Gfx/Font.h"

/**
 * @brief Present the initialization (version) screen
 *
 * This display shows the software and hardware revision and serial numbers on the display for a
 * bit, until either Menu is pressed, or the timer we set times out.
 */
void Task::showVersionScreen() {
    BaseType_t ok;

    // static labels
    static Gui::Components::StaticLabel gHelloLabel(
        {Gfx::MakePoint(4, 0), Gfx::MakeSize(248, 20)},
        "Programmable Load",
        Gfx::Font::gGeneral_16_Bold
    );

    static char gVersionString[50];
    snprintf(gVersionString, sizeof(gVersionString), "Software: %s/%s (%s)",
            gBuildInfo.gitBranch, gBuildInfo.gitHash, gBuildInfo.buildType);

    static Gui::Components::StaticLabel gSwVersionLabel(
        {Gfx::MakePoint(4, 46), Gfx::MakeSize(42, 18)},
        gVersionString,
        Gfx::Font::gGeneral_14
    );

    // present the screen
    static etl::array<Gui::Components::Base *, 2> gComponents{{
        &gHelloLabel, &gSwVersionLabel
    }};
    static Gui::Screen gVersionScreen{
        .title = "Version Info",
        .components = gComponents,
    };
    Gui::ScreenManager::Present(&gVersionScreen);

    // set up, and arm the timer
    static StaticTimer_t gDismissVersionTimer;
    this->versionDismissTimer = xTimerCreateStatic("Dismiss version screen",
        // one-shot timer mode
        pdMS_TO_TICKS(kShowVersionDuration), pdFALSE,
        // timer ID is this object
        this, [](auto timer) {
            xTaskNotifyIndexed(gShared->task, kNotificationIndex, TaskNotifyBits::ShowHomeScreen,
                    eSetBits);
        }, &gDismissVersionTimer);
    REQUIRE(this->versionDismissTimer, "pinball: %s", "failed to allocate version dismiss timer");

    ok = xTimerReset(this->versionDismissTimer, 0);
    REQUIRE(ok == pdTRUE, "pinball: %s", "failed to start version dismiss timer");
}
