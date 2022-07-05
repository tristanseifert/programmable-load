#include <FreeRTOS.h>

#include "stm32mp1xx_ll_gpio.h"
#include "stm32mp1xx_hal_hsem.h"

#include "Gpio.h"

#include "Log/Logger.h"
#include "Rtos/Rtos.h"

using namespace Drivers;

/**
 * @brief Get the bit corresponding to the given pin number
 *
 * Returns the GPIO pin constant for the given pin in the (port, pin) pair.
 */
constexpr static inline uint32_t GetPinBit(const Gpio::Pin pin) {
    switch(pin.second & 0xf) {
        case 0:
            return LL_GPIO_PIN_0;
        case 1:
            return LL_GPIO_PIN_1;
        case 2:
            return LL_GPIO_PIN_2;
        case 3:
            return LL_GPIO_PIN_3;
        case 4:
            return LL_GPIO_PIN_4;
        case 5:
            return LL_GPIO_PIN_5;
        case 6:
            return LL_GPIO_PIN_6;
        case 7:
            return LL_GPIO_PIN_7;
        case 8:
            return LL_GPIO_PIN_8;
        case 9:
            return LL_GPIO_PIN_9;
        case 10:
            return LL_GPIO_PIN_10;
        case 11:
            return LL_GPIO_PIN_11;
        case 12:
            return LL_GPIO_PIN_12;
        case 13:
            return LL_GPIO_PIN_13;
        case 14:
            return LL_GPIO_PIN_14;
        case 15:
            return LL_GPIO_PIN_15;
    }

    // should never get here
    return 0;
}

/**
 * @brief Get the GPIO instance corresponding to the given pin
 *
 * Returns the GPIO instance that corresponds to the port on which the given pin is located.
 */
constexpr static inline GPIO_TypeDef *GetPinPort(const Gpio::Pin &pin) {
    switch(pin.first) {
        case Gpio::Port::PortA:
            return GPIOA;
        case Gpio::Port::PortB:
            return GPIOB;
        case Gpio::Port::PortC:
            return GPIOC;
        case Gpio::Port::PortD:
            return GPIOD;
        case Gpio::Port::PortE:
            return GPIOE;
        case Gpio::Port::PortF:
            return GPIOF;
        case Gpio::Port::PortG:
            return GPIOG;
        case Gpio::Port::PortH:
            return GPIOH;
        case Gpio::Port::PortI:
            return GPIOI;
    }

    // should never get here
    return nullptr;
}


/**
 * @brief Configure an IO pin
 *
 * Sets up the pin as one of three categories: disabled, digital IO, or peripheral IO. According to
 * the values in the pin config struct, the pin is appropriately configured.
 *
 * @param pin IO pin to configure
 * @param config Detailed pin configuration
 */
void Gpio::ConfigurePin(const Pin pin, const PinConfig &config) {
    REQUIRE(pin.second <= 16, "invalid pin: %u", pin.second);
    const auto pinMask = GetPinBit(pin);

    // build up the pin struct; start with pin number
    LL_GPIO_InitTypeDef init{};
    init.Pin = pinMask;

    // drive strength
    switch(config.speed) {
        default: [[fallthrough]];
        case 0:
            init.Speed = LL_GPIO_SPEED_FREQ_LOW;
            break;
        case 1:
            init.Speed = LL_GPIO_SPEED_FREQ_MEDIUM;
            break;
        case 2:
            init.Speed = LL_GPIO_SPEED_FREQ_HIGH;
            break;
        case 3:
            init.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
            break;
    }

    // pull up/down mode
    switch(config.pull) {
        default: [[fallthrough]];
        case Pull::None:
            init.Pull = LL_GPIO_PULL_NO;
            break;

        case Pull::Up:
            init.Pull = LL_GPIO_PULL_UP;
            break;

        case Pull::Down:
            init.Pull = LL_GPIO_PULL_DOWN;
            break;
    }

    // open drain
    init.OutputType = config.isOpenDrain ? LL_GPIO_OUTPUT_OPENDRAIN : LL_GPIO_OUTPUT_PUSHPULL;

    // pin mode
    switch(config.mode) {
        case Mode::DigitalIn:
            init.Mode = LL_GPIO_MODE_INPUT;
            break;
        case Mode::DigitalOut:
            init.Mode = LL_GPIO_MODE_OUTPUT;
            break;
        case Mode::Analog:
            init.Mode = LL_GPIO_MODE_ANALOG;
            break;
        case Mode::Peripheral:
            init.Mode = LL_GPIO_MODE_ALTERNATE;
            // TODO: do we need to validate this?
            init.Alternate = config.function & 0xf;
            break;

        // pin is disabled (put it into analog mode)
        case Mode::Off:
            init.Mode = LL_GPIO_MODE_ANALOG;
            init.Pull = LL_GPIO_PULL_NO;
            init.Speed = LL_GPIO_SPEED_FREQ_LOW;
            break;
    }

    // perform the update
    auto port = GetPinPort(pin);
    AcquireLock();
    taskENTER_CRITICAL();

    if(config.mode == Mode::DigitalOut) {
        if(config.initialOutput) {
            LL_GPIO_SetOutputPin(port, pinMask);
        } else {
            LL_GPIO_ResetOutputPin(port, pinMask);
        }
    }
    LL_GPIO_Init(port, &init);

    UnlockGpio();
    taskEXIT_CRITICAL();
}

/**
 * @brief Set the state of an output pin
 *
 * Update the state driven onto an output pin.
 */
void Gpio::SetOutputState(const Pin pin, const bool state) {
    auto port = GetPinPort(pin);
    const auto pinMask = GetPinBit(pin);

    if(state) {
        LL_GPIO_SetOutputPin(port, pinMask);
    } else {
        LL_GPIO_ResetOutputPin(port, pinMask);
    }
}

/**
 * @brief Get the state of an input pin
 *
 * Read the input port register
 */
bool Gpio::GetInputState(const Pin pin) {
    auto port = GetPinPort(pin);
    const auto pinMask = GetPinBit(pin);

    return LL_GPIO_IsInputPinSet(port, pinMask);
}




/**
 * @brief Acquire GPIO configuration semaphore
 *
 * To ensure we don't conflict with accesses from the Linux side, we use the first hardware
 * semaphore slot to synchronize.
 */
void Gpio::AcquireLock() {
    bool ok{false};

    size_t tries{0};
    while(!ok && tries++ < kLockRetries) {
        auto ret = HAL_HSEM_FastTake(kSemaphoreId);
        ok = (ret == HAL_OK);
    }

    // failed to take the lock
    REQUIRE(ok, "failed to acquire gpio hwsem")
}

/**
 * @brief Release GPIO configuration semaphore
 *
 * Drops the semaphore that protects GPIO configuration accesses.
 */
void Gpio::UnlockGpio() {
    HAL_HSEM_Release(kSemaphoreId, 0);
}
