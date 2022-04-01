#include "Task.h"
#include "Hardware.h"
#include "../Control/Hardware.h"
#include "../Pinball/Hardware.h"
#include "../Pinball/Task.h"
#include "../Thermal/Hardware.h"
#include "../Thermal/Task.h"

#include "Log/Logger.h"
#include "Usb/Usb.h"

using namespace App::Main;

StaticTask_t Task::gTcb;
StackType_t Task::gStack[kStackSize];
Task *Task::gShared{nullptr};

/**
 * @brief Initialize the app main task.
 */
void App::Main::Start() {
    static uint8_t gTaskBuf[sizeof(Task)] __attribute__((aligned(4)));
    auto ptr = reinterpret_cast<Task *>(gTaskBuf);

    Task::gShared = new (ptr) Task();
}

/**
 * @brief Start app main task
 */
Task::Task() {
    this->task = xTaskCreateStatic([](void *ctx) {
        reinterpret_cast<Task *>(ctx)->main();
        Logger::Panic("what the fuck");
    }, kName.data(), kStackSize, this, kPriority, gStack, &gTcb);
}

/**
 * @brief Task entry point
 */
void Task::main() {
    int err;
    BaseType_t ok;
    uint32_t note;

    Logger::Debug("MainTask: %s", "start");

    // initialize onboard hardware, associated busses, and devices connected thereto
    this->initHardware();
    this->initNorFs();

    this->discoverIoHardware();
    this->discoverDriverHardware();

    // configure communication interfaces
    UsbStack::Init();

    // start additional app components
    this->startApp();

    // start processing messages
    vTaskPrioritySet(nullptr, kRuntimePriority);
    Logger::Debug("MainTask: %s", "start msg loop");

    while(1) {
        /*
         * Receive a task notification
         *
         * This is a 32-bit integer, where each bit indicates some event that we need to attend to;
         * these are each provided with their own handler functions, which may pull data from
         * additional queues.
         *
         * It's important that any handlers here don't block for extended amounts of time,
         * particularly when reading from queues for additional data.
         */
        ok = xTaskNotifyWaitIndexed(kNotificationIndex, 0,
                static_cast<uint32_t>(TaskNotifyBits::All), &note, portMAX_DELAY);
        REQUIRE(ok == pdTRUE, "%s failed: %d", "xTaskNotifyWaitIndexed", ok);

        // service IO bus interrupt
        if(note & TaskNotifyBits::IoBusInterrupt) {
            bool frontIrq{false}, rearIrq{false};
            err = Hw::QueryIoIrq(frontIrq, rearIrq);

            if(err) {
                Logger::Error("Failed to query IO bus irq: %d", err);
            }
            // notify the pinball task
            else {
                Pinball::Task::NotifyIrq(frontIrq, rearIrq);
            }
        }
    }
}

/**
 * @brief Initialize hardware
 *
 * Perform initialization of all low level hardware drivers on the board:
 *
 * - SERCOM0: Local I2C bus (used for front panel, rear IO)
 *   - PCA9543A: Multiplexes bus into two separate front/rear IO
 * - SERCOM3: Load driver board I2C bus
 * - SERCOM4: SPI for front panel display
 * - SERCOM5: SPI for local NOR flash
 * - TC5: PWM generation for beeper
 */
void Task::initHardware() {
    Logger::Debug("MainTask: %s", "init hw");

    // initialize the driver control bus
    Logger::Debug("MainTask: %s", "init driver i2c");
    Control::Hw::Init();

    /*
     * Initialize the local IO I²C bus
     *
     * This is multiplexed with a PCA9543A into a separate front IO and rear IO bus. The rear IO
     * bus is shared with some peripherals on the processor board.
     */
    etl::array<Drivers::I2CBus *, 2> ioBusses{};
    Logger::Debug("MainTask: %s", "init io i2c");

    Hw::InitIoBus();

    Logger::Debug("MainTask: %s", "init io i2c bus mux");
    Hw::InitIoBusMux(ioBusses);

    Logger::Trace("IO busses: %p %p", ioBusses[0], ioBusses[1]);

    // initialize user interface IO: display SPI, power button, encoder, beeper
    Logger::Debug("MainTask: %s", "init io spi");
    Pinball::Hw::Init(ioBusses);

    // TODO: initialize NOR flash SPI
    Logger::Debug("MainTask: %s", "init nor spi");

    /*
     * Initialize onboard peripherals.
     *
     * This consists of the following:
     *
     * - EMC2101-R: Fan controller (rear IO bus; address 0b100'1100)
     */
    Thermal::Hw::InitFanController(ioBusses[1]);
}

/**
 * @brief Initialize NOR filesystem
 *
 * Sets up the littlefs filesystem against the just initialized SPI NOR flash. This flash contains
 * system configuration data, which later parts of the startup process will need to consult.
 *
 * The SPI flash is a AT25SF321, but the actual type isn't really important as long as it follows
 * the JEDEC command assignments.
 */
void Task::initNorFs() {
    Logger::Debug("MainTask: %s", "init nor fs");
    // TODO: implement
}

/**
 * @brief Discover connected front/rear IO hardware
 *
 * Scan both the front and rear IO I2C bus for devices. This is accomplished by searching for a
 * serial number EEPROM (AT24CS32 type) and attempting to read out its contents, which hold a
 * board id that we can then look up in a table to figure out what drivers are needed.
 */
void Task::discoverIoHardware() {
    Logger::Debug("MainTask: %s", "discover io hw");
    // TODO: implement
}

/**
 * @brief Discover connected drivers
 *
 * Scan the driver board bus for a configuration EEPROM (AT24CS32 type) and parse its contents the
 * same way as the front/rear IO boards. Based on the board ID discovered, we'll launch the
 * appropriate driver.
 *
 * @remark Currently, only a single driver board is supported; this may change at some point in
 *         the future.
 */
void Task::discoverDriverHardware() {
    Logger::Debug("MainTask: %s", "discover driver hw");
    // TODO: implement
}

/**
 * @brief Start application tasks
 *
 * Called once all hardware and drivers have been initialized, and all services are available. It
 * will start the application tasks:
 *
 * - Thermal management (fan control, overheat protection)
 * - Pinball (front panel UI)
 * - Control loop
 */
void Task::startApp() {
    Logger::Debug("MainTask: %s", "start app");

    Thermal::Start();
    Pinball::Start();
}
