#include <vendor/sam.h>

#include "Log/Logger.h"

__attribute__( ( naked ) )
void HardFault_Handler() {
        __asm volatile
    (
        " tst lr, #4                                                \n"
        " ite eq                                                    \n"
        " mrseq r0, msp                                             \n"
        " mrsne r0, psp                                             \n"
        " ldr r1, [r0, #24]                                         \n"
        " ldr r2, handler2_address_const                            \n"
        " bx r2                                                     \n"
        " handler2_address_const: .word prvGetRegistersFromStack    \n"
    );
    //Logger::Panic("Hard fault!");
}

__attribute__((used))
extern "C" void prvGetRegistersFromStack( uint32_t *pulFaultStackAddress )
{
/* These are volatile to try and prevent the compiler/linker optimising them
away as the variables never actually get used.  If the debugger won't show the
values of the variables, make them global my moving their declaration outside
of this function. */
volatile uint32_t r0;
volatile uint32_t r1;
volatile uint32_t r2;
volatile uint32_t r3;
volatile uint32_t r12;
volatile uint32_t lr; /* Link register. */
volatile uint32_t pc; /* Program counter. */
volatile uint32_t psr;/* Program status register. */

    r0 = pulFaultStackAddress[ 0 ];
    r1 = pulFaultStackAddress[ 1 ];
    r2 = pulFaultStackAddress[ 2 ];
    r3 = pulFaultStackAddress[ 3 ];

    r12 = pulFaultStackAddress[ 4 ];
    lr = pulFaultStackAddress[ 5 ];
    pc = pulFaultStackAddress[ 6 ];
    psr = pulFaultStackAddress[ 7 ];

    Logger::Panic("Hard Fault!\n r0 %08x  r1 %08x  r2 %08x  r3 %08x\n"
            "r12 %08x  lr %08x  pc %08x psr %08x\n"
            "hfsr %08x",
            r0, r1, r2, r3, r12, lr, pc, psr, *((uint32_t *) 0xe000ed2c));

    /* When the following line is hit, the variables contain the register values. */
    for( ;; );
}

extern "C" void __cxa_pure_virtual() {
    Logger::Panic("pure virtual called");
}
