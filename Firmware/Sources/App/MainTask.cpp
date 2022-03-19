#include "MainTask.h"

#include "Log/Logger.h"
#include "Usb/Usb.h"

using namespace App;

StaticTask_t MainTask::gTcb;
StackType_t MainTask::gStack[kStackSize];

/**
 * @brief Initialize the app main task.
 */
void MainTask::Start() {
    static uint8_t gTaskBuf[sizeof(MainTask)] __attribute__((aligned(4)));
    auto ptr = reinterpret_cast<MainTask *>(gTaskBuf);

    new (ptr) MainTask();
}

/**
 * @brief Start app main task
 */
MainTask::MainTask() {
    this->task = xTaskCreateStatic([](void *ctx) {
        reinterpret_cast<MainTask *>(ctx)->main();
        Logger::Panic("what the fuck");
    }, kName.data(), kStackSize, this, kPriority, gStack, &gTcb);
}

/**
 * @brief Task entry point
 */
void MainTask::main() {
    // initialize onboard hardware, associated busses, and devices connected thereto
    this->initHardware();
    this->initOnboardPeripherals();
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
        // TODO: implement
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/**
 * @brief Initialize hardware
 *
 * Perform initialization of all low level hardware drivers on the board:
 *
 * - SERCOM0: Local I2C bus (used for front panel, rear IO; multiplexed with PCA9543A)
 * - SERCOM3: Load driver board I2C bus
 * - SERCOM4: SPI for front panel display
 * - SERCOM5: SPI for local NOR flash
 * - TC5: PWM generation for beeper
 */
void MainTask::initHardware() {
    Logger::Debug("MainTask: %s", "init hw");

    // TODO: initialize the IO I2C bus
    Logger::Debug("MainTask: %s", "init io i2c");

    // initialize user interface IO: display SPI, power button
    Logger::Debug("MainTask: %s", "init io spi");

    // TODO: initialize NOR flash SPI
    Logger::Debug("MainTask: %s", "init nor spi");

    // TODO: initialize driver I2C bus
    Logger::Debug("MainTask: %s", "init driver i2c");

    // TODO: timer for beeper
}

/**
 * @brief Initialize onboard peripherals
 *
 * Using the previously initialized busses, initialize some on-board peripherals. This consists of
 * the following:
 *
 * - Front/rear IO bus multiplexer (PCA9543A)
 *   - On rear IO bus: Fan controller (EMC2101-R)
 * - SPI NOR flash (AT25SF321)
 */
void MainTask::initOnboardPeripherals() {
    Logger::Debug("MainTask: %s", "init onboard periph");
    // TODO: implement
}

/**
 * @brief Initialize NOR filesystem
 *
 * Sets up the littlefs filesystem against the just initialized SPI NOR flash. This flash contains
 * system configuration data, which later parts of the startup process will need to consult.
 */
void MainTask::initNorFs() {
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
void MainTask::discoverIoHardware() {
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
void MainTask::discoverDriverHardware() {
    Logger::Debug("MainTask: %s", "discover driver hw");
    // TODO: implement
}

/**
 * @brief Start application tasks
 *
 * Called once all hardware and drivers have been initialized, and all services are available. It
 * will start the application tasks:
 *
 * - Pinball (front panel UI)
 * - Control loop
 */
void MainTask::startApp() {
    Logger::Debug("MainTask: %s", "start app");
    // TODO: implement
}
