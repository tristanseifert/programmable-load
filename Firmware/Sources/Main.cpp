#include <hpl_gpio.h>

#include "App/Main/Task.h"
#include "Drivers/ExternalIrq.h"
#include "Log/Logger.h"
#include "Rtos/Start.h"

#include "Drivers/Gpio.h"

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
    static const Drivers::Gpio::PinConfig kLedOutput{
        .mode = Drivers::Gpio::Mode::DigitalOut,
    };

    Drivers::Gpio::ConfigurePin({Drivers::Gpio::Port::PortA, 3}, kLedOutput); // B
    Drivers::Gpio::ConfigurePin({Drivers::Gpio::Port::PortB, 4}, kLedOutput); // G
    Drivers::Gpio::ConfigurePin({Drivers::Gpio::Port::PortB, 5}, kLedOutput); // R

    Drivers::Gpio::SetOutputState({Drivers::Gpio::Port::PortA, 3}, 1); // B
    Drivers::Gpio::SetOutputState({Drivers::Gpio::Port::PortB, 4}, 0); // G
    Drivers::Gpio::SetOutputState({Drivers::Gpio::Port::PortB, 5}, 1); // R

    Logger::Warning("Hello world!");


    /*
     * Do early hardware initialization
     *
     * These are peripherals that are embedded in the processor and will always be present and
     * used for something.
     */
    Drivers::ExternalIrq::Init();

    /*
     * Create the main app task
     *
     * This task is responsible for performing additional application initialization. Once the
     * initialization stage is over, the task handles the user interface.
     */
    App::Main::Start();

    // then, start the scheduler
    Rtos::StartScheduler();

    // XXX: blink status LED
    uint8_t temp{1};
    while(1) {
        // write them
//        Drivers::Gpio::SetOutputState({Drivers::Gpio::Port::PortA, 3}, !!(temp & 0b100)); // B
        Drivers::Gpio::SetOutputState({Drivers::Gpio::Port::PortB, 4}, !!(temp & 0b010)); // G
        Drivers::Gpio::SetOutputState({Drivers::Gpio::Port::PortB, 5}, !!(temp & 0b001)); // R

        // advance
        temp++;
        volatile uint32_t n{0};
        do {
            n = n + 1;
        } while(n != 2000000);
    }

    // should never get here...
    return 0;
}
