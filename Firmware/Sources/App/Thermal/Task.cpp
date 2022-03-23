#include "Task.h"
#include "Hardware.h"

#include "Drivers/I2CBus.h"
#include "Drivers/I2CDevice/EMC2101.h"
#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include <etl/algorithm.h>
#include <etl/mean.h>
#include <etl/numeric.h>

using namespace App::Thermal;

StaticTask_t Task::gTcb;
StackType_t Task::gStack[kStackSize];

Task *Task::gShared{nullptr};

/**
 * @brief Start the thermal management task task
 *
 * This initializes the shared thermal management task instance.
 */
void App::Thermal::Start() {
    static uint8_t gTaskBuf[sizeof(Task)] __attribute__((aligned(alignof(Task))));
    auto ptr = reinterpret_cast<Task *>(gTaskBuf);

    Task::gShared = new (ptr) Task();
}

/**
 * @brief Initialize the task
 */
Task::Task() {
    // initialize some data
    etl::fill(this->fanSpeeds.begin(), this->fanSpeeds.end(), -1);
    etl::fill(this->sensorTemps.begin(), this->sensorTemps.end(), -9999);

    // initialize locks
    this->sensorsLock = xSemaphoreCreateMutexStatic(&this->sensorsLockStorage);
    this->fansLock = xSemaphoreCreateMutexStatic(&this->fansLockStorage);

    /*
     * Register on-board sensors and fans.
     *
     * This requires that before the task is started, all hardware relevant to temperature sensing
     * and fan control is initialized.
     */
    this->sensors.emplace_back(Sensor{
        etl::delegate<int(float &)>::create<Drivers::I2CDevice::EMC2101,
                &Drivers::I2CDevice::EMC2101::getInternalTemp>(*Hw::gFanController),
        "CPU board",
        // a case interior temp of 60°C is quite spicy
        60,
    });

    this->fans.emplace_back(Fan{
        etl::delegate<int(const uint8_t)>::create<Drivers::I2CDevice::EMC2101,
                &Drivers::I2CDevice::EMC2101::setFanSpeed>(*Hw::gFanController),
        etl::delegate<int(int &)>::create<Drivers::I2CDevice::EMC2101,
                &Drivers::I2CDevice::EMC2101::getFanSpeed>(*Hw::gFanController),
        "Case Rear",
        // its speed is manually controlled by firmware
        false,
    });

    // then create the task
    this->task = xTaskCreateStatic([](void *ctx) {
        reinterpret_cast<Task *>(ctx)->main();
        Logger::Panic("what the fuck");
    }, kName.data(), kStackSize, this, kPriority, gStack, &gTcb);
}

/**
 * @brief Clean up the task
 * Tear down resources 
 */
Task::~Task() {
    vSemaphoreDelete(this->sensorsLock);
    vSemaphoreDelete(this->fansLock);
}

/**
 * @brief Main loop
 */
void Task::main() {
    BaseType_t ok;
    int err;

    /*
     * Process the main loop.
     *
     * We'll sample all thermal sensors one after another, then update fans.
     */
    while(1) {
        /*
         * Read all sensors in the order they were registered. Each of the sensors' readings will
         * be stored in our cache.
         */
        bool failedToRead{false};
        ok = xSemaphoreTake(this->sensorsLock, pdMS_TO_TICKS(10));

        const size_t numSensors{this->sensors.size()};

        if(ok == pdTRUE) {
            // read them
            for(size_t i = 0; i < numSensors; i++) {
                float temp;
                const auto &sense = this->sensors[i];
                err = sense.callback(temp);

                if(!err) {
                    this->sensorTemps[i] = temp;
                } else {
                    Logger::Warning("failed to read %s %u: %d", "temp", i, err);
                    failedToRead = true;
                    continue;
                }
            }

            // decrement the failsafe counter
            if(this->failsafeCount) {
                if(!--this->failsafeCount && !failedToRead) {
                    // if it's reached zero, exit failsafe mode
                    this->failsafeMode = false;
                }
            }

            // be sure to release the sensor lock
            xSemaphoreGive(this->sensorsLock);
        }
        /*
         * We either failed to acquire the lock to the sensor data, or failed to read a sensor.
         */
        if(ok != pdTRUE || failedToRead) {
            // enter failsafe mode if not yet there
            if(!this->failsafeMode) {
                if(++this->failsafeCount == kFailsafeThreshold) {
                    this->failsafeMode = true;
                }
            }
        }

        /*
         * Update the state of fans.
         *
         * We'll read each fan's speed, caching it like we do with the temperature sensor data;
         * then we adjust all fans that require manual control.
         *
         * This basically just consists of the rear panel fan, which we'll control primarily based
         * on the overall ambient case temperature, but also taking into account the temperature
         * reported by the driver board's fan controller (on the heatsink)
         */
        // calculate the desired fan speed (average all sensors)
        uint8_t desiredSpeed{0};

        float meanTemp{0.f};
        for(size_t i = 0; i < numSensors; i++) {
            meanTemp += this->sensorTemps[i];
        }
        meanTemp = meanTemp / static_cast<float>(numSensors);

        if(meanTemp <= 0.f || this->failsafeMode) {
            // failsafe mode is enabled for invalid readings also
            desiredSpeed = 0xff;
        }
        else {
            // above 35°C, enable the fan, running at maximum speed at 50°C
            if(meanTemp >= 35.f) {
                float speedPct = ((meanTemp - 30.f) * .05f);
                if(speedPct > 1.f) speedPct = 1.f;

                desiredSpeed = static_cast<uint8_t>(speedPct * 255);
            } else {
                desiredSpeed = 0;
            }
        }

        // read and update fans
        ok = xSemaphoreTake(this->fansLock, pdMS_TO_TICKS(10));
        if(ok == pdTRUE) {
            for(size_t i = 0; i < this->fans.size(); i++) {
                int speed;
                const auto &fan = this->fans[i];

                // update fan (if not in automatic mode)
                if(!fan.isAutomatic) {
                    err = fan.setCallback(desiredSpeed);
                    if(err) {
                        Logger::Warning("failed to write %s %u: %d", "fan speed", i, err);
                    }
                }

                // read its speed
                err = fan.getCallback(speed);
                if(!err) {
                    this->fanSpeeds[i] = speed;
                } else {
                    Logger::Warning("failed to read %s %u: %d", "fan speed", i, err);
                    continue;
                }
            }
            // be sure to release the lock
            xSemaphoreGive(this->fansLock);
        }

        // XXX: testing
        int speed;
        if(this->readFanSpeed(0, speed)) {
            Logger::Notice("Mean temp: %d °C, fan 0 %u", static_cast<int>(meanTemp), speed);
        }

        float temp;
        if(this->readTemperatureSensor(0, temp)) {
            Logger::Notice("Temp 0: %d", (int) temp);
        }

        // finished this iteration, wait for next
        vTaskDelay(pdMS_TO_TICKS(kLoopInterval));
    }
}
