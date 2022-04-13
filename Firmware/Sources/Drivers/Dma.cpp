#include "Dma.h"
#include "Common.h"

#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include <string.h>
#include <etl/algorithm.h>
#include <etl/array.h>
#include <vendor/sam.h>

using namespace Drivers;

DmacDescriptor Dma::gDescriptors[kNumChannels] __attribute__((aligned(8)));
DmacDescriptor Dma::gWritebackDescriptors[kNumChannels] __attribute__((aligned(8)));
etl::array<TaskHandle_t, Dma::kNumChannels> Dma::gBlockedTasks;
etl::array<int, Dma::kNumChannels> Dma::gCompletionReason;

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

    // set up concurrency support
    etl::fill(gBlockedTasks.begin(), gBlockedTasks.end(), nullptr);

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
 * @brief Enable a DMA channel
 *
 * Set the channel's enable flag. The transfer configuration should have been set previously.
 *
 * @param DMA channel to enable
 */
void Dma::EnableChannel(const uint8_t channel) {
    REQUIRE(channel < kNumChannels, "DMAC: invalid channel (%u)", channel);

    auto &regs = DMAC->Channel[channel];
    regs.CHCTRLA.bit.ENABLE = 1;
}

/**
 * @brief Configure a DMA channel
 *
 * This sets up shared transfer characteristics for the channel, such as the FIFO threshold, burst
 * length, and trigger sources.
 *
 * @remark The channel must be disabled to configure it.
 *
 * @param channel DMA channel number to configure
 * @param threshold  FIFO transfer threshold
 * @param burstLength Length of a burst transfer ([0, 15])
 * @param trigger Action to take when an external trigger event is received
 * @param triggerSource Source for DMA trigger; set to 0 to use only software triggers
 * @param priority Channel priority ([0, 3]; numerically high values = highest priority)
 */
void Dma::ConfigureChannel(const uint8_t channel, const FifoThreshold threshold,
                const uint8_t burstLength, const TriggerAction trigger,
                const uint8_t triggerSource, const uint8_t priority) {
    REQUIRE(channel < kNumChannels, "DMAC: invalid channel (%u)", channel);

    auto &regs = DMAC->Channel[channel];

    // build up CHCTRLA
    regs.CHCTRLA.reg = DMAC_CHCTRLA_THRESHOLD(static_cast<uint8_t>(threshold) & 0b11) |
        DMAC_CHCTRLA_BURSTLEN(burstLength & 0xf) |
        DMAC_CHCTRLA_TRIGACT(static_cast<uint8_t>(trigger) & 0b11) |
        DMAC_CHCTRLA_TRIGSRC(triggerSource);// | DMAC_CHCTRLA_ENABLE;

    // set the priority
    regs.CHPRILVL.bit.PRILVL = priority & 0b11;

    // enable channel transfer complete + error irq's
    regs.CHINTENSET.reg = DMAC_CHINTENSET_TCMPL | DMAC_CHINTENSET_TERR;
}

/**
 * @brief Disable a DMA channel
 *
 * Clears the DMA's enable flag, which will cancel any in-progress transfers. The call returns only
 * when the channel is fully disabled.
 *
 * @param DMA channel to disable
 */
void Dma::DisableChannel(const uint8_t channel) {
    REQUIRE(channel < kNumChannels, "DMAC: invalid channel (%u)", channel);

    auto &regs = DMAC->Channel[channel];
    regs.CHCTRLA.bit.ENABLE = 0;

    while(regs.CHCTRLA.bit.ENABLE) {}
}

/**
 * @brief Reset a DMA channel
 *
 * Disable the channel, then perform a software reset. All channel registers are cleared to their
 * initial state, and the channel will remain disabled.
 *
 * @param DMA channel to reset
 */
void Dma::ResetChannel(const uint8_t channel) {
    REQUIRE(channel < kNumChannels, "DMAC: invalid channel (%u)", channel);

    // ensure the channel is disabled
    DisableChannel(channel);

    // then, execute sw reset and wait for completion
    auto &regs = DMAC->Channel[channel];
    regs.CHCTRLA.bit.SWRST = 1;

    while(regs.CHCTRLA.bit.SWRST) {}
}

/**
 * @brief Configure a DMA transfer descriptor
 *
 * Sets up a channel's DMA transfer descriptor with the provided transfer source, destination, and
 * other configuration values.
 *
 * @remark Once the descriptor is configured, the transfer will begin with the next trigger, which
 *         may be a software trigger.
 *
 * @param channel DMA channel to update the descriptor for
 * @param size Size of a beat (single bus cycle)
 * @param source Address to read data from
 * @param srcIncrement Whether the source address is incremented after each transfer
 * @param destination Destination memory address
 * @param destIncrement Whether the destination address is incremented after each transfer
 * @param transferLength Total size of the transfer, in bytes. Must be evenly divisible by the
 *        beat size, and result in less than \f$2^16-1\f$ transfers total.
 *
 * @return 0 on success, an error code otherwise
 */
int Dma::ConfigureTransfer(const uint8_t channel, const BeatSize size, const void *source,
        const bool srcIncrement, void *destination, const bool destIncrement,
        const size_t transferLength) {
    REQUIRE(channel < kNumChannels, "DMAC: invalid channel (%u)", channel);

    // validate transfer size
    size_t numBeats{0};

    switch(size) {
        case BeatSize::Byte:
            numBeats = transferLength;
            break;
        case BeatSize::HalfWord:
            if(transferLength & 0b1) {
                return Errors::LengthBeatMismatch;
            }
            numBeats = transferLength / 2;
            break;
        case BeatSize::Word:
            if(transferLength & 0b11) {
                return Errors::LengthBeatMismatch;
            }
            numBeats = transferLength / 4;
            break;
    }

    if(numBeats > 0xffff) {
        return Errors::TooLong;
    }

    // ensure channel is disabled
    auto &desc = gDescriptors[channel];
    desc.BTCTRL.bit.VALID = 0;
    __DSB();

    // configure channel control
    desc.BTCTRL.bit.SRCINC = srcIncrement ? 1 : 0;
    desc.BTCTRL.bit.DSTINC = destIncrement ? 1 : 0;
    desc.BTCTRL.bit.BEATSIZE = static_cast<uint8_t>(size) & 0b11;

    desc.BTCTRL.bit.BLOCKACT = 0x01; // disable after transfer, raise interrupt

    desc.BTCNT.reg = static_cast<uint16_t>(numBeats);

    // configure source and destination
    if(srcIncrement) {
        desc.SRCADDR.reg = reinterpret_cast<uintptr_t>(source) + transferLength;
    } else {
        desc.SRCADDR.reg = reinterpret_cast<uintptr_t>(source);
    }

    if(destIncrement) {
        desc.DSTADDR.reg = reinterpret_cast<uintptr_t>(destination) + transferLength;
    } else {
        desc.DSTADDR.reg = reinterpret_cast<uintptr_t>(destination);
    }

    // no linked descriptor
    desc.DESCADDR.reg = 0;

    // lastly, enable the descriptor again
    __DSB();
    desc.BTCTRL.bit.VALID = 1;

    // if we get here, the transfer is ok
    return 0;
}

/**
 * @brief Trigger a DMA channel transfer
 *
 * Generate a software DMA trigger for the specified channel.
 *
 * @param channel DMA channel to trigger
 */
void Dma::Trigger(const uint8_t channel) {
    REQUIRE(channel < kNumChannels, "DMAC: invalid channel (%u)", channel);

    DMAC->SWTRIGCTRL.reg = (1U << static_cast<uintptr_t>(channel));
}

/**
 * @brief Wait for the specified DMA channel to complete
 *
 * Block the calling task until the specified DMA channel transfer completes or fails.
 *
 * @param channel DMA channel to wait on
 *
 * @return Transfer status code, where ≥ 0 indicates success
 */
int Dma::WaitForCompletion(const uint8_t channel) {
    uint32_t note;
    int err{0};
    BaseType_t ok;

    // prepare state
    taskENTER_CRITICAL();
    gBlockedTasks[channel] = xTaskGetCurrentTaskHandle();
    gCompletionReason[channel] = -1;
    taskEXIT_CRITICAL();

    // wait
    ok = xTaskNotifyWaitIndexed(Rtos::TaskNotifyIndex::DriverPrivate, 0,
            Drivers::NotifyBits::DmaController, &note, portMAX_DELAY);
    if(!ok) {
        err = Errors::BlockError;
        goto beach;
    }

    // grab actual completion reason
    err = gCompletionReason[channel];

beach:;
    // return state
    taskENTER_CRITICAL();
    gBlockedTasks[channel] = nullptr;
    taskEXIT_CRITICAL();

    return err;
}



/**
 * @brief Interrupt handler
 *
 * Handles an interrupt for the DMA channel specified.
 */
void Dma::HandleIrq(const uint8_t channel) {
    auto &regs = DMAC->Channel[channel];
    const auto intflag = regs.CHINTFLAG.reg;

    // transfer complete?
    if(intflag & DMAC_CHINTFLAG_TCMPL) {
        SignalChannelComplete(channel);
    }
    // check for channel errors
    else if(intflag & DMAC_CHINTFLAG_TERR) {
        const auto status = regs.CHSTATUS.reg;

        if(status & DMAC_CHSTATUS_FERR) {
            SignalChannelComplete(channel, Errors::InvalidDescriptor);
        } else {
            SignalChannelComplete(channel, Errors::TransferError);
        }
    }

    // clear the interrupt status
    regs.CHINTFLAG.reg = intflag;
}

/**
 * @brief Wake blocked task
 *
 * Notifies any task blocking on this DMA channel that the transfer completed.
 */
void Dma::SignalChannelComplete(const uint8_t channel, const int status) {
    BaseType_t woken{pdFALSE};

    // update stored status
    gCompletionReason[channel] = status;
    __DSB();

    // notify waiting task, if any
    auto task = gBlockedTasks[channel];
    if(!task) return;

    xTaskNotifyIndexedFromISR(task, Rtos::TaskNotifyIndex::DriverPrivate,
            Drivers::NotifyBits::DmaController, eSetBits, &woken);
    portYIELD_FROM_ISR(woken);
}


/**
 * @brief DMA channel 0 interrupt handler
 */
void DMAC_0_Handler(void) {
    Dma::HandleIrq(0);
}

/**
 * @brief DMA channel 1 interrupt handler
 */
void DMAC_1_Handler(void) {
    Dma::HandleIrq(1);
}

/**
 * @brief DMA channel 2 interrupt handler
 */
void DMAC_2_Handler(void) {
    Dma::HandleIrq(2);
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
