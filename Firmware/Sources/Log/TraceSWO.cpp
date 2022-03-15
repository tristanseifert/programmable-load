#include "TraceSWO.h"

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
    TPI->FFCR = 0x00000100u;
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
