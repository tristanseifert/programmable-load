#include "TraceSWO.h"

#include "Drivers/Gpio.h"

#include <vendor/sam.h>

#include <stddef.h>

using namespace Log;

/**
 * @brief Initialize Trace SWO output
 *
 * This sets up the SWO port; some debuggers do this during attachment but this ensures the
 * interface is available.
 *
 * @param cpuFreq CPU frequency, in Hz
 *
 * @todo Is this really necessary if we have the debugger initializing the SWO registers?
 */
void TraceSWO::Init(const uint32_t cpuFreq) {
    // configure PB30 as SWO output: alternate function "H"
    Drivers::Gpio::ConfigurePin({Drivers::Gpio::Port::PortB, 30}, {
        .mode = Drivers::Gpio::Mode::Peripheral,
        .function = MUX_PB30H_CM4_SWO
    });

    // enable ETIM clock (register GCLK_CM4_TRACE) (GCLK0, 120MHz)
    GCLK->PCHCTRL[47].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;

    // calculate baud rate prescaler based on CPU frequency
    constexpr static const uint32_t kBaudRate{100000};
    const uint32_t prescale = (cpuFreq / kBaudRate) - 1;

    // enable trace functionality
    CoreDebug->DEMCR = CoreDebug_DEMCR_TRCENA_Msk;
    // use SWO as output and set the prescaler for the desired baud rate
    TPI->SPPR = (0x2 << TPI_SPPR_TXMODE_Pos);
    TPI->ACPR = prescale;
    // enable write access to ITM control register
    ITM->LAR = 0xC5ACCE55u;

    // enable ATBID, ITM enable, SYNC enable, DWT enable
    ITM->TCR	= 0x0001000Du;
    // enable all stimulus ports
    ITM->TPR = ITM_TPR_PRIVMASK_Msk;
    // enable tracing on port 1
    ITM->TER = 0b1;
    // data watchpoint and trace config
    DWT->CTRL = 0x400003FEu;
    TPI->FFCR = 0x00000100u;*/

#define CPU_FREQUENCY 120000000UL
#define SWO_FREQUENCY 6000000UL
#define SWOPRESCALE (((cpuFreq)/(SWO_FREQUENCY)) - 1)


    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // enable access to the trace component registers.
    TPI->SPPR = 0x2UL; // Selected Pin Protocol Register -> Serial Wire Viewer, UART NRZ 
    TPI->ACPR = 4;
    ITM->LAR = 0xC5ACCE55UL; // unlock the ITM
    ITM->TCR = ITM_TCR_ITMENA_Msk | ITM_TCR_TSENA_Msk | (1UL << ITM_TCR_TraceBusID_Pos)	| ITM_TCR_DWTENA_Msk | ITM_TCR_SYNCENA_Msk | ITM_TCR_SWOENA_Msk; // Enable ITM
    ITM->TER = 0xFFFFFFFF; // ITM Trace Enable Register 
    ITM->TPR = 0;
}

/**
 * @brief Output character to Trace SWO
 *
 * Dumps the specified character to the SWO output.
 *
 * @param ch Character to output
 */
void TraceSWO::PutChar(const char ch) {
    ITM_SendChar(ch);
}
