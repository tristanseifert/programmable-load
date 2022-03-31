#include "Hardware.h"
#include "Task.h"

#include "Drivers/ExternalIrq.h"
#include "Drivers/Gpio.h"
#include "Drivers/Spi.h"
#include "Drivers/TimerCounter.h"
#include "Log/Logger.h"
#include "Rtos/Rtos.h"


#include <vendor/sam.h>

using namespace App::Pinball;

Drivers::Spi *Hw::gDisplaySpi{nullptr};
Drivers::TimerCounter *Hw::gBeeperTc{nullptr};

Drivers::I2CBus *Hw::gFrontI2C{nullptr};
Drivers::I2CBus *Hw::gRearI2C{nullptr};

/**
 * @brief Initialize user interface hardware
 *
 * Initializes the display SPI interface, various GPIOs for the encoder, front panel control, the
 * timer for generating beeps, and the power button.
 *
 * @param busses A list containing two I²C bus instances: the first for the front panel, the latter
 *        for the rear IO.
 */
void Hw::Init(const etl::span<Drivers::I2CBus *, 2> &busses) {
    gFrontI2C = busses[0];
    gRearI2C = busses[1];

    InitDisplaySpi();
    InitPowerButton();
    InitMisc();
    InitEncoder();
    InitBeeper();
    InitMisc();
}

/**
 * @brief Initialize the display SPI driver
 *
 * Initialize SERCOM4 with DOPO=2, DIPO=0 so that the following pins are used:
 * -  /CS: PB14/PAD2
 * -  SCK: PB13/PAD1
 * - MOSI: PB15/PAD3
 * - MISO: PB12/PAD0
 *
 * Additionally, the display command/data output is configured (PA04)
 */
void Hw::InitDisplaySpi() {
    /**
     * Set up the display command/data IO.
     */
    Drivers::Gpio::ConfigurePin(kDisplayCmdData, {
        .mode = Drivers::Gpio::Mode::DigitalOut,
        .pull = Drivers::Gpio::Pull::Up,
        .initialOutput = 0,
    });

    /*
     * Configure the SPI IO lines as peripheral mode.
     *
     * Though MISO is configured for peripheral use, the receiver is disabled since the display
     * does not send out any data.
     */
    // PAD0: SPI master in, slave out (MISO)
    Drivers::Gpio::ConfigurePin(kDisplayMiso, {
        .mode = Drivers::Gpio::Mode::Peripheral,
        .function = MUX_PB12C_SERCOM4_PAD0,
    });
    // PAD1: SPI clock (SCK)
    Drivers::Gpio::ConfigurePin(kDisplaySck, {
        .mode = Drivers::Gpio::Mode::Peripheral,
        .function = MUX_PB13C_SERCOM4_PAD1,
    });
    // PAD2:  Chip select
    Drivers::Gpio::ConfigurePin(kDisplayCs, {
        .mode = Drivers::Gpio::Mode::Peripheral,
        .function = MUX_PB14C_SERCOM4_PAD2,
    });
    // PAD3: SPI master out, slave in (MOSI)
    Drivers::Gpio::ConfigurePin(kDisplayMosi, {
        .mode = Drivers::Gpio::Mode::Peripheral,
        .function = MUX_PB15C_SERCOM4_PAD3,
    });

    /*
     * Configure the SPI with mode = 3, and a 10MHz frequency.
     */
    static const Drivers::Spi::Config cfg{
        .rxEnable = 0,
        .inputPin = 0,
        .alternateOutput = 1,
        .sckFrequency = 10'000'000,
    };

    static uint8_t gSpiBuf[sizeof(Drivers::Spi)] __attribute__((aligned(alignof(Drivers::Spi))));
    auto ptr = reinterpret_cast<Drivers::Spi *>(gSpiBuf);
    gDisplaySpi = new (ptr) Drivers::Spi(Drivers::SercomBase::Unit::Unit4, cfg);
}

/**
 * @brief Initialize the IOs for the power button.
 *
 * The power button is an illuminated tactile switch on the main board; it puts the device into a
 * low power standby mode when pushed. Its IO allocations are as follows:
 *
 * - Switch input: PB31 (active low)
 * -   Switch LED: PA27
 */
void Hw::InitPowerButton() {
    /*
     * Configure the power button input, with a weak pull up. Also set up an external interrupt
     * that's triggered on a falling edge.
     */
    Drivers::Gpio::ConfigurePin(kPowerSwitch, {
        .mode = Drivers::Gpio::Mode::DigitalIn,
        .pull = Drivers::Gpio::Pull::Up,
        .function = MUX_PB31A_EIC_EXTINT15,
        .pinMuxEnable = 1,
    });

    // enable irq on falling edge of power button
    Drivers::ExternalIrq::ConfigureLine(15, {
        .irq = 1,
        .event = 0,
        .filter = 1,
        .debounce = 1,
        .mode = Drivers::ExternalIrq::SenseMode::EdgeFalling
    });

    NVIC_SetPriority(EIC_15_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 3);
    NVIC_EnableIRQ(EIC_15_IRQn);

    // illuminate with the primary mode
    SetPowerLight(PowerLightMode::Primary);
}

/**
 * @brief Initialize the rotary encoder
 *
 * Set up the inputs for the rotary encoder:
 * - A: PB07 (active low)
 * - B: PB08 (active low)
 *
 * Each of these is also set up for external interrupts on both the rising and falling edge.
 */
void Hw::InitEncoder() {
    // TODO: reset the encoder state machine

    // configure encoder A and B inputs
    Drivers::Gpio::ConfigurePin(kEncoderA, { // A
        .mode = Drivers::Gpio::Mode::DigitalIn,
        .pull = Drivers::Gpio::Pull::Up,
        .function = MUX_PB07A_EIC_EXTINT7,
        .pinMuxEnable = 1,
    });
    Drivers::Gpio::ConfigurePin(kEncoderB, { // B
        .mode = Drivers::Gpio::Mode::DigitalIn,
        .pull = Drivers::Gpio::Pull::Up,
        .function = MUX_PB08A_EIC_EXTINT8,
        .pinMuxEnable = 1,
    });

    // configure the external interrupts
    Drivers::ExternalIrq::ConfigureLine(7, { // A
        .irq = 1,
        .event = 0,
        .filter = 1,
        .debounce = 1,
        .mode = Drivers::ExternalIrq::SenseMode::EdgeBoth
    });
    Drivers::ExternalIrq::ConfigureLine(8, { // B
        .irq = 1,
        .event = 0,
        .filter = 1,
        .debounce = 1,
        .mode = Drivers::ExternalIrq::SenseMode::EdgeBoth
    });

    // enable interrupt vectors
    NVIC_SetPriority(EIC_7_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 2);
    NVIC_SetPriority(EIC_8_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 2);

    NVIC_EnableIRQ(EIC_7_IRQn);
    NVIC_EnableIRQ(EIC_8_IRQn);
}

/**
 * @brief Initialize the beeper
 *
 * The beeper is a magnetic transducer, which should be driven at roughly 2.4kHz square wave. These
 * tones are generated by Timer/Counter 5, operating in PWM mode. It is connected to PB10.
 */
void Hw::InitBeeper() {
    // configure IO
    Drivers::Gpio::ConfigurePin(kBeeper, {
        .mode = Drivers::Gpio::Mode::Peripheral,
        .function = MUX_PB10E_TC5_WO0,
    });

    // configure the counter/timer
    static const Drivers::TimerCounter::Config cfg{
        .countDown = 0,
        .stop = 1,
        .invertWo0 = 0,
        .wavegen = Drivers::TimerCounter::WaveformMode::NPWM,
        .frequency = 2400,
    };

    static uint8_t gTcBuf[sizeof(Drivers::TimerCounter)]
        __attribute__((aligned(alignof(Drivers::TimerCounter))));
    auto ptr = reinterpret_cast<Drivers::TimerCounter *>(gTcBuf);
    gBeeperTc = new (ptr) Drivers::TimerCounter(Drivers::TimerCounter::Unit::Tc5, cfg);

    // test beep
    gBeeperTc->setDutyCycle(0, .1);
    vTaskDelay(pdMS_TO_TICKS(200));
    gBeeperTc->setDutyCycle(0, 0);
}

/**
 * @brief Initialize miscellaneous IOs
 *
 * This configures various IOs for the user interface:
 *
 * - /FRONT_RESET: PA5 (external pull-up)
 */
void Hw::InitMisc() {
    // front panel reset line
    Drivers::Gpio::ConfigurePin(kFrontIoReset, {
        .mode = Drivers::Gpio::Mode::DigitalOut,
        .initialOutput = 1,
    });
}



/**
 * @brief Reset the front panel
 *
 * This asserts the reset line to the front panel for at least 100ms. Connected to this line are
 * all peripherals on the front IO board, as well as the display.
 */
void Hw::ResetFrontPanel() {
    Drivers::Gpio::SetOutputState(kFrontIoReset, false);
    vTaskDelay(pdMS_TO_TICKS(100));
    Drivers::Gpio::SetOutputState(kFrontIoReset, true);
}

/**
 * @brief Set the state of the display `D/C` line
 *
 * @param isData Whether the next transaction contains data or commands
 */
void Hw::SetDisplayDataCommandFlag(const bool isData) {
    Drivers::Gpio::SetOutputState(kDisplayCmdData, isData);
}

/**
 * @brief Set the state of the power light
 *
 * The light is controlled by either driving the pin high or low (for the two colors) or floating
 * it to disable the light.
 *
 * @param mode Light mode to enable
 */
void Hw::SetPowerLight(const PowerLightMode mode) {
    // float IO to disable
    if(mode == PowerLightMode::Off) {
        Drivers::Gpio::ConfigurePin(kPowerIndicator, {
            .mode = Drivers::Gpio::Mode::Off,
        });
    } else {
        Drivers::Gpio::ConfigurePin(kPowerIndicator, {
            .mode = Drivers::Gpio::Mode::DigitalOut,
            .initialOutput = static_cast<uint8_t>((mode == PowerLightMode::Primary) ? 1 : 0),
        });
    }
}

/**
 * @brief Read the state of the two encoder pins.
 *
 * @return Encoder state; bit 0 is the A line, bit 1 the B line.
 */
uint8_t Hw::ReadEncoder() {
    uint8_t temp{0};
    if(Drivers::Gpio::GetInputState(kEncoderA)) {
        temp |= (1 << 0);
    }
    if(Drivers::Gpio::GetInputState(kEncoderB)) {
        temp |= (1 << 1);
    }
    return temp;
}



/**
 * @brief Interrupt handler for encoder A
 *
 * Indicates an edge was detected on the encoder's A line. Notify the user interface task to sample
 * the encoder and transition the state machine appropriately.
 */
void EIC_7_Handler() {
    BaseType_t woken{pdFALSE};

    if(Drivers::ExternalIrq::HandleIrq(7)) {
        Task::NotifyFromIsr(Task::TaskNotifyBits::EncoderChanged, &woken);
    }

    portYIELD_FROM_ISR(woken);
}

/**
 * @brief Interrupt handler for encoder B
 *
 * Indicates an edge was detected on the encoder's B line. Notify the user interface task to sample
 * the encoder and transition the state machine appropriately.
 */
void EIC_8_Handler() {
    BaseType_t woken{pdFALSE};

    if(Drivers::ExternalIrq::HandleIrq(8)) {
        Task::NotifyFromIsr(Task::TaskNotifyBits::EncoderChanged, &woken);
    }

    portYIELD_FROM_ISR(woken);
}

/**
 * @brief Interrupt handler for power button
 *
 * Indicates that the power button has been pressed, so send a notification to the user interface
 * task that this happened.
 */
void EIC_15_Handler() {
    BaseType_t woken{pdFALSE};

    if(Drivers::ExternalIrq::HandleIrq(15)) {
        Task::NotifyFromIsr(Task::TaskNotifyBits::PowerPressed, &woken);
    }

    portYIELD_FROM_ISR(woken);
}

