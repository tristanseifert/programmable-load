#ifndef APP_THERMAL_TASK_H
#define APP_THERMAL_TASK_H

#include <stddef.h>
#include <stdint.h>

#include "Rtos/Rtos.h"

#include <etl/array.h>
#include <etl/delegate.h>
#include <etl/unordered_multimap.h>
#include <etl/string_view.h>
#include <etl/vector.h>

/// Thermal management
namespace App::Thermal {
/**
 * @brief Thermal management task
 *
 * Responsible for querying various thermal sensors in the system, and controlling fans. Data is
 * periodically sampled.
 */
class Task {
    friend void Start();

    public:
        /**
         * @brief Callback to read a temperature sensor
         *
         * This function is invoked to read a particular temperature sensor. It should return 0
         * on success, or an error code; and populate the referenced float variable with the read
         * temperature, in °C, on success.
         */
        using TempReadCallback = etl::delegate<int(float &)>;

        /**
         * @brief Callback to set a fan's desired speed
         *
         * This function is invoked to set a particular registered fan's speed.
         *
         * It should return 0 on success, or an error code if the speed could not be set. Its only
         * argument is a byte, which is linearly proportional to the fan speed: a value of 0
         * indicates a 0% fan speed, a value of 0xFF indicates 100% (full) speed.
         */
        using FanSetSpeedCallback = etl::delegate<int(const uint8_t)>;

        /**
         * @brief Callback to get a fan's current rotational speed
         *
         * Retrieves the rotational speed of a fan, in RPM.
         *
         * It should return 0 on success, an error code otherwise. Its only argument is an integer
         * reference, which should be written with the fan's speed, in RPM. If reading fan speeds
         * is not supported, the variable should be set to -1.
         */
        using FanGetRpmCallback = etl::delegate<int(int &)>;

    public:
        /**
         * @brief Get temperature management task
         *
         * @return Pointer to the system's thermal management task instance
         */
        static inline auto The() {
            return gShared;
        }

        /**
         * @brief Register a temperature sensor
         *
         * This installs a sensor into the list of sensors available to the temperature monitor.
         * All sensors are used to consider the overall thermal state of the device for driving
         * case fans.
         *
         * @param callback Method to invoke to read the sensor
         * @param name Display name for the sensor
         * @param tempLimit Maximum temperature acceptable for this sensor, beyond which the
         *        system should enter thermal limiting mode
         * @param outIndex Variable to receive the sensor's registration index on success
         *
         * @return 0 on success, an error code otherwise
         */
        int registerSensor(TempReadCallback callback, const etl::string_view &name,
                const float tempLimit, size_t &outIndex) {
            int err{0};

            // acquire the sensors lock
            if(!xSemaphoreTake(this->sensorsLock, portMAX_DELAY)) {
                return -1;
            }

            // ensure there's space for another sensor, and then insert it
            if(this->sensors.full()) {
                err = -2;
                goto beach;
            }

            this->sensors.emplace_back(Sensor{
                callback, name, tempLimit
            });
            outIndex = this->sensors.size() - 1;

beach:;
            // release lock
            xSemaphoreGive(this->sensorsLock);
            return err;
        }

        /**
         * @brief Register a fan
         *
         * Install a fan into the list of fan controllers.
         *
         * @param setCallback Method to invoke to set the fan speed
         * @param getCallback Method to invoke to get the fan's current speed
         * @param name Display name for the fan
         * @param isAutomatic Whether the fan is automatically controlled
         * @param outIndex Variable to receive the fan's registration index on success
         *
         * @return 0 on success, an error code otherwise
         */
        int registerFan(FanSetSpeedCallback setCallback, FanGetRpmCallback getCallback,
                const etl::string_view &name, const bool isAutomatic, size_t &outIndex) {
            int err{0};

            // acquire the fans lock
            if(!xSemaphoreTake(this->fansLock, portMAX_DELAY)) {
                return -1;
            }

            // ensure there's space for another sensor, and then insert it
            if(this->fans.full()) {
                err = -2;
                goto beach;
            }

            this->fans.emplace_back(Fan{
                setCallback, getCallback, name, isAutomatic
            });
            outIndex = this->fans.size() - 1;

beach:;
            // release lock
            xSemaphoreGive(this->fansLock);
            return err;
        }

        /**
         * @brief Get the temperature for a particular sensor.
         *
         * @param sensor Sensor index
         * @param outTemp Variable to receive the temperature value
         *
         * @return Whether the temperature was read or not
         */
        constexpr inline bool readTemperatureSensor(const size_t sensor, float &outTemp) {
            if(sensor > kMaxSensors || sensor > this->sensors.size()) {
                return false;
            }

            outTemp = this->sensorTemps[sensor];
            return true;
        }

        /**
         * @brief Get the speed of a particular fan
         *
         * @param fan Fan index
         * @param outSpeed Variable to receive the fan speed, in RPM
         *
         * @return Whether the speed was read or not
         */
        constexpr inline bool readFanSpeed(const size_t fan, int &outSpeed) {
            if(fan > kMaxFans || fan > this->fans.size()) {
                return false;
            }

            outSpeed = this->fanSpeeds[fan];
            return true;
        }

    private:
        /**
         * @brief Temperature control interval
         *
         * Defines the duration, in milliseconds, between each invocation of the thermal control
         * loop. This samples all temperature sensors, and updates fan controllers accordingly.
         *
         * Additionally, this is the minimum interval used to detect overheating. Take care that
         * it's not possible to actually destroy everything in the time between iterations of
         * the control loop.
         */
        static const constexpr uint32_t kLoopInterval{740};

        /**
         * @brief Number of loops without data befoore entering failsafe mode
         *
         * This defines the maximum amount of time temperature sensing can be broken before we
         * enter the failsafe mode. Likewise, we need to have sensors back for this many periods
         * before failsafe mode is exited.
         */
        static const constexpr uint8_t kFailsafeThreshold{5};

        /**
         * @brief Maximum supported thermal sensors
         *
         * Provides an upper bound on the maximum number of thermal sensors that the task can
         * deal with. This reserves memory for handling them.
         *
         * @remark Large numbers of sensors can slow down the controller loop significantly.
         */
        static const constexpr size_t kMaxSensors{6};

        /**
         * @brief Maximum supported fans
         *
         * An upper bound on the number of fans in the system.
         */
        static const constexpr size_t kMaxFans{3};

        /// Runtime priority level
        static const constexpr uint8_t kPriority{Rtos::TaskPriority::AppHigh};

        /// Size of the task's stack, in words
        static const constexpr size_t kStackSize{350};
        /// Task name (for display purposes)
        static const constexpr etl::string_view kName{"Hotstuff"};
        /// Notification index
        static const constexpr size_t kNotificationIndex{Rtos::TaskNotifyIndex::TaskSpecific};

    private:
        /**
         * @brief Information about a system fan
         */
        struct Fan {
            /**
             * @brief Set speed callback
             */
            FanSetSpeedCallback setCallback;

            /**
             * @brief Read RPM callback
             */
            FanGetRpmCallback getCallback;

            /**
             * @brief Fan name
             *
             * Display name for this fan
             */
            const etl::string_view name;

            /**
             * @brief Use automatic control
             *
             * When set, this fan is automatically controlled, and the controller does not need to
             * update its speed.
             */
            bool isAutomatic{false};
        };

        /**
         * @brief Information about a temperature sensor in the system
         */
        struct Sensor {
            /**
             * @brief Callback to read the sensor
             *
             * It's invoked in the context of the thermal management task. The temperature should
             * be provided in °C, and the callback return 0 for success.
             */
            TempReadCallback callback;

            /**
             * @brief Sensor name
             *
             * Display name for this thermal sensor
             */
            const etl::string_view name;

            /**
             * @brief Upper limit temperature
             *
             * When the temperature exceeds this value, the system should enter an overheat state.
             */
            float overtempLimit;
        };

    private:
        Task();
        ~Task();

        void main();

    private:
        /// Task handle
        TaskHandle_t task;

        /**
         * @brief Current sensor temperatures
         *
         * The most recently read temperatures from each sensor, or NaN if there is no data
         * available.
         */
        etl::array<float, kMaxSensors> sensorTemps;

        /**
         * @brief Thermal sensors
         *
         * A list of all thermal sensors in the system.
         */
        etl::vector<Sensor, kMaxSensors> sensors;
        /// Thermal sensors lock
        SemaphoreHandle_t sensorsLock;
        /// Storage for the sensors lock
        StaticSemaphore_t sensorsLockStorage;

        /**
         * @brief Current fan speeds (RPM)
         *
         * Speeds of all fans in the system, as most recently read during the last loop of the
         * thermal control algorithm.
         */
        etl::array<int, kMaxFans> fanSpeeds;

        /**
         * @brief Fans
         *
         * All fans in the system
         */
        etl::vector<Fan, kMaxFans> fans;
        /// Thermal fans lock
        SemaphoreHandle_t fansLock;
        /// Storage for the fans lock
        StaticSemaphore_t fansLockStorage;

        /**
         * @brief Failsafe mode
         *
         * Whether the thermal control system is in failsafe mode, because sensors aren't
         * providing valid data. In this mode, all fans run at maximum speed.
         */
        bool failsafeMode{false};
        /**
         * @brief Failsafe sample count
         *
         * Number of consecutive times we were unable to get temperature data.
         */
        uint8_t failsafeCount{0};

    private:
        /// Task information structure
        static StaticTask_t gTcb;
        /// Preallocated stack for the task
        static StackType_t gStack[kStackSize];

        /// Shared task instance
        static Task *gShared;
};

void Start();
}

#endif
