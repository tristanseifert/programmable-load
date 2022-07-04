//#include "Log/Logger.h"
//#include "Rtos/Start.h"
//#include "Util/HwInfo.h"

#include "BuildInfo.h"

#include "stm32mp1xx_hal_gpio.h"
#include "stm32mp1xx_hal_rcc.h"

#include "LockResource.h"

//#include <timers.h>

volatile int zero = 0;
volatile int blaze = 420;

static void SetRgbLed(const bool r, const bool g, const bool b) {
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_5, r ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, g ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, b ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

/**
 * @brief Early hardware init
 *
 * This un-gates various clocks and enables some basic peripherals (RCC, GPIO) that we will need
 * throughout the life of the software.
 */
static void EarlyHwInit() {
    // enable clocks for RCC, GPIO
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    // enable hardware semaphores
    __HAL_RCC_HSEM_CLK_ENABLE();
}

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
    EarlyHwInit();

    /*
    Logger::Warning("\n\n**********\nProgrammable load fw (%s/%s-%s)\n%s@%s, on %s)",
            gBuildInfo.gitBranch, gBuildInfo.gitHash, gBuildInfo.buildType,
            gBuildInfo.buildUser, gBuildInfo.buildHost, gBuildInfo.buildDate);
    */

    (void) zero;
    (void) blaze;

    int fucker = zero + blaze;
    (void) fucker;

    // set up GPIOs for RGB LED
    GPIO_InitTypeDef gpioShit{
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_MEDIUM,
    };

    gpioShit.Pin = GPIO_PIN_8; // PF8, blue
    HAL_GPIO_Init(GPIOF, &gpioShit);
    gpioShit.Pin = GPIO_PIN_13; // PD13, green
    HAL_GPIO_Init(GPIOD, &gpioShit);
    gpioShit.Pin = GPIO_PIN_5; // PG5, red
    HAL_GPIO_Init(GPIOG, &gpioShit);

    // set RGB LED state
    SetRgbLed(false, true, false);

    /*
     * Last, transfer control to the FreeRTOS scheduler. This will begin executing the app main
     * task, which in turn performs all initialization with full scheduling services available.
     *
     * We do it this way so that our initialization can rely on stuff like timers working.
     */
    //Rtos::StartScheduler();
    uint8_t balls{1};
    while(1) {
        volatile uint32_t shit{0x800000};
        while(shit--) {}

        balls++;
        SetRgbLed(balls & 0x8, balls & 0x4, balls & 0x2);
    }

    return -1;
}
