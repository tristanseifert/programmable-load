/**
 * \file
 *
 * \brief Initial entry point
 *
 */

#include "vendor/same51.h"
#include "Log/TraceSWO.h"

extern "C" {

/* Initialize segments */
extern uint32_t _sfixed;
extern uint32_t _efixed;
extern uint32_t _etext;
extern uint32_t _srelocate;
extern uint32_t _erelocate;
extern uint32_t _szero;
extern uint32_t _ezero;
extern uint32_t _sstack;
extern uint32_t _estack;

/** \cond DOXYGEN_SHOULD_SKIP_THIS */
int main(void);
/** \endcond */

extern void _init_chip();

/**
 * @brief CPU core clock
 *
 * This variable is set to the frequency of the CPU core, in Hz.
 */
uint32_t SystemCoreClock{0};
}

static void InvokeConstructors();

/**
 * @brief Reset handler
 *
 * This initializes the device (namely, setting up RAM regions, vector tables, etc.) and then
 * jumps to our main function.
 */
extern "C" void Reset_Handler(void) {
    uint32_t *pSrc, *pDest;

    // copy read/write .data from flash to RAM
    pSrc  = &_etext;
    pDest = &_srelocate;

    if (pSrc != pDest) {
        for (; pDest < &_erelocate;) {
            *pDest++ = *pSrc++;
        }
    }

    // clear .bss
    for (pDest = &_szero; pDest < &_ezero;) {
        *pDest++ = 0;
    }

    // set vector table base
    pSrc      = (uint32_t *)&_sfixed;
    SCB->VTOR = ((uint32_t)pSrc & SCB_VTOR_TBLOFF_Msk);

#if __FPU_USED
    // enable FPU if it's used
    SCB->CPACR |= (0xFu << 20);
    __DSB();
    __ISB();
#endif

    /*
     * Set up clocks
     *
     * This configures the external 12MHz high frequency crystal, a PLL based off this to generate
     * the 120MHz CPU clock, one to generate a 48MHz USB clock, and the 32.768kHz crystal to use
     * for the RTC.
     */
    _init_chip();
    SystemCoreClock = 120'000'000u; // TODO: can we figure this automatically?

    // set up SWO output
    Log::TraceSWO::Init(SystemCoreClock);

    // run C library initializers
    InvokeConstructors();

    // jump to main function; abort if return
    main();

    while (1) {
        __BKPT(0xff);
    }
}

/**
 * @brief Invoke initializer functions
 *
 * Runs all initializer functions that are stored in the `init_array` section in the executable,
 * for functions marked as constructors, and static variables.
 */
static void InvokeConstructors() {
    extern void (*__init_array_start)();
    extern void (*__init_array_end)();

    for (void (**p)() = &__init_array_start; p < &__init_array_end; ++p) {
        (*p)();
    }
}
