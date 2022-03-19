#include <hpl_gpio.h>

#include "App/MainTask.h"
#include "Log/Logger.h"
#include "Rtos/Start.h"

/**
 * @brief Application entry point
 *
 * We'll jump here after the chip is mostly set up; that is, the RAM regions are established. We
 * should then perform low-level hardware initialization (clocks, peripherals, IOs) and set up the
 * tasks and other OS resources.
 *
 * Lastly, the RTOS scheduler is launched to actually begin execution.
 */
extern "C" int main() {
    /*
     * Configure the RGB status LED, as well as the power button and its associated indicator
     * light, to show some early indications of life.
     */
    _gpio_set_direction(GPIO_PORTA, GPIO_PIN(3), GPIO_DIRECTION_OUT); // B
    _gpio_set_direction(GPIO_PORTB, GPIO_PIN(4), GPIO_DIRECTION_OUT); // G
    _gpio_set_direction(GPIO_PORTB, GPIO_PIN(5), GPIO_DIRECTION_OUT); // R

    _gpio_set_level(GPIO_PORTA, GPIO_PIN(3), 1); // B
    _gpio_set_level(GPIO_PORTB, GPIO_PIN(4), 0); // G
    _gpio_set_level(GPIO_PORTB, GPIO_PIN(5), 1); // R

    Logger::Warning("Hello world!");

    /*
     * Create the main app task
     *
     * This task is responsible for performing additional application initialization. Once the
     * initialization stage is over, the task handles the user interface.
     */
    App::MainTask::Start();

    // then, start the scheduler
    Rtos::StartScheduler();

    // XXX: blink status LED
    uint8_t temp{1};
    while(1) {
        // write them
        _gpio_set_level(GPIO_PORTA, GPIO_PIN(3), !!(temp & 0b100)); // B
        _gpio_set_level(GPIO_PORTB, GPIO_PIN(4), !!(temp & 0b010)); // G
        _gpio_set_level(GPIO_PORTB, GPIO_PIN(5), !!(temp & 0b001)); // R

        // advance
        temp++;
        volatile uint32_t n{0};
        do {
            n = n + 1;
        } while(n != 20000000);
    }

    // should never get here...
    return 0;
}
