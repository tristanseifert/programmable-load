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

uint8_t Hw::gEncoderCurrentState{0};
int Hw::gEncoderDelta{0};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"

const uint8_t Hw::kEncoderStateTable[7][4]{
    // Initial
    {Initial,   CwBegin,        CcwBegin,       Initial},
    // CwFinal
    {CwNext,    Initial,        CwFinal,        (Initial | DirectionCW)},
    // CwBegin
    {CwNext,    CwBegin,        Initial,        Initial},
    // CwNext
    {CwNext,    CwBegin,        CwFinal,        Initial},
    // CcwBegin
    {CcwNext,   Initial,        CcwBegin,       Initial},
    // CcwFinal
    {CcwNext,   CcwFinal,       Initial,        (Initial | DirectionCCW)},
    // CcwNext
    {CcwNext,   CcwFinal,       CcwBegin,       Initial},
};

#pragma GCC diagnostic pop

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
    InitStatus();
    InitPowerButton();
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
    // PAD3: SPI master out, slave in (MOSI)
    Drivers::Gpio::ConfigurePin(kDisplayMosi, {
        .mode = Drivers::Gpio::Mode::Peripheral,
        .function = MUX_PB15C_SERCOM4_PAD3,
    });

    // chip select is under manual control
    Drivers::Gpio::ConfigurePin(kDisplayCs, {
        .mode = Drivers::Gpio::Mode::DigitalOut,
        .initialOutput = 1,
    });

    /*
     * Configure the SPI with mode = 3, and a 10MHz frequency.
     */
    static const Drivers::Spi::Config cfg{
        .cpol = 1,
        .cpha = 1,
        .rxEnable = 0,
        .hwChipSelect = 0,
        .useDma = 1,
        .dmaChannelTx = 2,
        .dmaPriorityTx = 1,
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
 *
 * @TODO Update this for the rev2 pcb (the LED was changed)
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

    NVIC_SetPriority(EIC_15_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 4);
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
    // reset the encoder state machine
    gEncoderCurrentState = EncoderState::Initial;

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
    NVIC_SetPriority(EIC_7_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 4);
    NVIC_SetPriority(EIC_8_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 4);

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
 * @brief Initialize the on-board RGB status LED
 *
 * The following IOs are configured:
 *
 * - STATUS_R: PB05
 * - STATUS_G: PB04
 * - STATUS_B: PA03
 *
 * Pulling them low will illuminate the LED.
 */
void Hw::InitStatus() {
    static const Drivers::Gpio::PinConfig kLedOutput{
        .mode = Drivers::Gpio::Mode::DigitalOut,
        .initialOutput = 1,
    };

    Drivers::Gpio::ConfigurePin(kStatusLedR, kLedOutput);
    Drivers::Gpio::ConfigurePin(kStatusLedG, kLedOutput);
    Drivers::Gpio::ConfigurePin(kStatusLedB, kLedOutput);
}

/**
 * @brief Sets the status LED indicator
 *
 * This is an RGB LED on the processor board. It's intended as a quick diagnostic aid; therefore
 * no effort has gone towards making this work with PWM, for example.
 *
 * @param color Color to set the status indicator; bit order is 0b0000'0RGB
 */
void Hw::SetStatusLed(const uint8_t color) {
    Drivers::Gpio::SetOutputState(kStatusLedR, !(color & 0b100));
    Drivers::Gpio::SetOutputState(kStatusLedG, !(color & 0b010));
    Drivers::Gpio::SetOutputState(kStatusLedB, !(color & 0b001));
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
 * @brief Update rotary encoder state machine
 *
 * Reads the IO pins of the rotary encoder, and updates the state machine for it. When a full left
 * or right click is detected, we'll notify the pinball task.
 */
void Hw::AdvanceEncoderState(BaseType_t *woken) {
    const auto state = Hw::ReadEncoder();

    gEncoderCurrentState = kEncoderStateTable[gEncoderCurrentState & 0xf][state];
    const auto direction = gEncoderCurrentState & EncoderDirection::DirectionMask;

    // one "step" has taken place now
    if(direction) {
        if(direction == EncoderDirection::DirectionCW) {
            __atomic_add_fetch(&gEncoderDelta, 1, __ATOMIC_RELAXED);
        } else if(direction == EncoderDirection::DirectionCCW) {
            __atomic_sub_fetch(&gEncoderDelta, 1, __ATOMIC_RELAXED);
        }

        Task::NotifyFromIsr(Task::TaskNotifyBits::EncoderChanged, woken);
    }
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
        Hw::AdvanceEncoderState(&woken);
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
        Hw::AdvanceEncoderState(&woken);
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

