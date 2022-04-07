#include "Dma.h"

#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include <string.h>
#include <etl/array.h>
#include <vendor/sam.h>

using namespace Drivers;

DmacDescriptor Dma::gDescriptors[kNumChannels] __attribute__((aligned(8)));
DmacDescriptor Dma::gWritebackDescriptors[kNumChannels] __attribute__((aligned(8)));

/**
 * @brief Initialize the DMA controller
 *
 * Configures the required clocks and interrupts for the DMA controller.
 */
void Dma::Init() {
    // enable clocks
    MCLK->AHBMASK.bit.DMAC_ = true;

    // reset the controller
    DMAC->CTRL.bit.DMAENABLE = 0;
    DMAC->CTRL.bit.SWRST = 1;
    while(DMAC->CTRL.bit.SWRST != 0) {}

    // configure the descriptor and write-back bases
    memset(gDescriptors, 0, sizeof(gDescriptors));
    memset(gWritebackDescriptors, 0, sizeof(gWritebackDescriptors));

    DMAC->BASEADDR.reg = reinterpret_cast<uint32_t>(&gDescriptors);
    DMAC->WRBADDR.reg = reinterpret_cast<uint32_t>(&gWritebackDescriptors);

    /*
     * Configure (and enable) each of the four priority levels, where 0 is the highest:
     *
     * 0. Critical QoS (real time)
     * 1. Medium QoS (user interactive)
     * 2. Low QoS (general hardware stuff)
     * 3. Background (no QoS)
     */
    DMAC->PRICTRL0.reg =
        (DMAC_PRICTRL0_RRLVLEN3 | DMAC_PRICTRL0_QOS3_CRITICAL) |
        (DMAC_PRICTRL0_RRLVLEN2 | DMAC_PRICTRL0_QOS2_SENSITIVE) |
        (DMAC_PRICTRL0_RRLVLEN1 | DMAC_PRICTRL0_QOS1_SHORTAGE) |
        (DMAC_PRICTRL0_RRLVLEN0 | DMAC_PRICTRL0_QOS0_REGULAR);
    DMAC->CTRL.vec.LVLEN = 0b1111;

    // configure and enable the DMAC interrupts
    constexpr static const etl::array<const IRQn_Type, 5> kIrqs{{
        DMAC_0_IRQn, DMAC_1_IRQn, DMAC_2_IRQn,
        DMAC_3_IRQn, DMAC_4_IRQn,
    }};

    for(const auto irqn : kIrqs) {
        NVIC_SetPriority(irqn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 2);
        NVIC_EnableIRQ(irqn);
    }

    // then, enable the DMAC and reset all channels
    DMAC->CTRL.bit.DMAENABLE = 1;

    for(size_t i = 0; i < kNumChannels; i++) {
        auto &channel = DMAC->Channel[i];
        channel.CHCTRLA.bit.SWRST = 1;
    }
}



/**
 * @brief DMA channel 0 interrupt handler
 */
void DMAC_0_Handler(void) {
    // TODO: implement
    Logger::Panic("DMAC: unhandled irq %u (%08x)", 0, DMAC->INTSTATUS.reg);
}

/**
 * @brief DMA channel 1 interrupt handler
 */
void DMAC_1_Handler(void) {
    // TODO: implement
    Logger::Panic("DMAC: unhandled irq %u (%08x)", 1, DMAC->INTSTATUS.reg);
}

/**
 * @brief DMA channel 2 interrupt handler
 */
void DMAC_2_Handler(void) {
    // TODO: implement
    Logger::Panic("DMAC: unhandled irq %u (%08x)", 2, DMAC->INTSTATUS.reg);
}

/**
 * @brief DMA channel 3 interrupt handler
 */
void DMAC_3_Handler(void) {
    // TODO: implement
    Logger::Panic("DMAC: unhandled irq %u (%08x)", 3, DMAC->INTSTATUS.reg);
}

/**
 * @brief DMA channel 4-31 interrupt handler
 */
void DMAC_4_Handler(void) {
    // TODO: implement
    Logger::Panic("DMAC: unhandled irq %u (%08x)", 4, DMAC->INTSTATUS.reg);
}
