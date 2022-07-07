#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include "stm32mp1xx.h"
#include "stm32mp1xx_hal_ipcc.h"
#include "stm32mp1xx_hal_rcc.h"

#include <openamp/open_amp.h>

#include "Mailbox.h"
#include "Rpc.h"

using namespace Rpc;

IPCC_HandleTypeDef Mailbox::gHandle;
Mailbox::ChannelStatus Mailbox::gStatus[2];
size_t Mailbox::gMissedIrqs[2];

TaskHandle_t Mailbox::gNotifyTask{nullptr};
size_t Mailbox::gNotifyIndex{0};
uintptr_t Mailbox::gVirtioNotifyBits{0};
uintptr_t Mailbox::gShutdownNotifyBits{0};

/**
 * @brief Initialize the mailbox
 *
 * This enables all clocks and then configures the IPCC peripheral.
 */
void Mailbox::Init() {
    // enable clocks, configure irq's
    __HAL_RCC_IPCC_CLK_ENABLE();

    NVIC_SetPriority(IPCC_RX1_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 2);
    NVIC_SetPriority(IPCC_TX1_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 2);

    // initialize IPCC HAL driver and its callbacks
    gHandle.Instance = IPCC;

    auto status = HAL_IPCC_Init(&gHandle);
    REQUIRE(status == HAL_OK, "%s failed: %d", "HAL_IPCC_Init", status);

    InstallCallbacks();

    // enable interrupts
    Logger::Notice("IPCC enabled");

    NVIC_EnableIRQ(IPCC_RX1_IRQn);
    NVIC_EnableIRQ(IPCC_TX1_IRQn);
}

/**
 * @brief Install IPCC callbacks
 */
void Mailbox::InstallCallbacks() {
    HAL_StatusTypeDef status;

    // install ch1 callback (M4 -> A7)
    status = HAL_IPCC_ActivateNotification(&gHandle, IPCC_CHANNEL_1, IPCC_CHANNEL_DIR_RX,
            [](auto hipcc, auto channel, auto dir) {
        if(gStatus[0] != ChannelStatus::Idle) {
            Logger::Warning("%s: %s (%u)", "IPCC M4->A7", "missed irq", ++gMissedIrqs[0]);
        }

        gStatus[0] = ChannelStatus::RxBufferFreed;

        // request work
        if(gNotifyTask) {
            BaseType_t woken{pdFALSE};
            xTaskNotifyIndexedFromISR(gNotifyTask, gNotifyIndex, gVirtioNotifyBits, eSetBits, &woken);
            portYIELD_FROM_ISR(woken);
        }

        // acknowledge message
        HAL_IPCC_NotifyCPU(hipcc, channel, IPCC_CHANNEL_DIR_RX);
    });
    REQUIRE(status == HAL_OK, "%s failed: %d", "HAL_IPCC_ActivateNotification", status);

    // install ch2 callback (A7 -> M4)
    status = HAL_IPCC_ActivateNotification(&gHandle, IPCC_CHANNEL_2, IPCC_CHANNEL_DIR_RX,
            [](auto hipcc, auto channel, auto dir) {
        if(gStatus[1] != ChannelStatus::Idle) {
            Logger::Warning("%s: %s (%u)", "IPCC A7->M4", "missed irq", ++gMissedIrqs[1]);
        }

        gStatus[1] = ChannelStatus::RxBufferAvailable;

        // request work
        if(gNotifyTask) {
            BaseType_t woken{pdFALSE};
            xTaskNotifyIndexedFromISR(gNotifyTask, gNotifyIndex, gVirtioNotifyBits, eSetBits, &woken);
            portYIELD_FROM_ISR(woken);
        }

        // acknowledge reception
        HAL_IPCC_NotifyCPU(hipcc, channel, IPCC_CHANNEL_DIR_RX);
    });
    REQUIRE(status == HAL_OK, "%s failed: %d", "HAL_IPCC_ActivateNotification", status);

    // install Ch3 callback (shutdown request)
    status = HAL_IPCC_ActivateNotification(&gHandle, IPCC_CHANNEL_3, IPCC_CHANNEL_DIR_RX,
            [](auto hipcc, auto channel, auto dir) {
        // notify the message handler task
        if(gNotifyTask) {
            BaseType_t woken{pdFALSE};
            xTaskNotifyIndexedFromISR(gNotifyTask, gNotifyIndex, gShutdownNotifyBits, eSetBits,
                    &woken);
            portYIELD_FROM_ISR(woken);
        }

        // do NOT acknowledge the message yet; we'll get turned off as soon as we do
        // HAL_IPCC_NotifyCPU(hipcc, IPCC_CHANNEL_3, IPCC_CHANNEL_DIR_RX);
    });
    REQUIRE(status == HAL_OK, "%s failed: %d", "HAL_IPCC_ActivateNotification", status);
}



/**
 * @brief Process a deferred interrupt
 *
 * Handles deferred interrupts (that is, indicators of a free buffer, or a newly arrived message)
 * in the context of the calling task.
 *
 * @param vdev Virtio device owning the buffers
 *
 * @return Number of events processed, or -1 if none
 */
int Mailbox::ProcessDeferredIrq(struct virtio_device *vdev) {
    int status{-1};

    // handle ch1 messages (A7 has processed message and released buffer)
    if(gStatus[0] == ChannelStatus::RxBufferFreed) {
        rproc_virtio_notified(vdev, 0/* VRING0_ID */);

        // clear its status
        gStatus[0] = ChannelStatus::Idle;
        status = 0;
    }

    // handle ch2 messages (message available)
    if(gStatus[1] == ChannelStatus::RxBufferAvailable) {
        rproc_virtio_notified(vdev, 1/* VRING1_ID */);

        // clear state
        gStatus[1] = ChannelStatus::RxBufferAvailable;
        status = 0;

        // XXX: copied from ST sample code, why do we do this?
        // The OpenAMP framework does not notify for free buf: do it here
        rproc_virtio_notified(nullptr, 1/* VRING_ID */);
    }

    return status;
}

/**
 * @brief Notify remote host of activity
 *
 * Ring the appropriate doorbell to notify the remote host that something happened.
 *
 * @param priv Context ptr (unused)
 * @param id VRing ID to notify
 */
int Mailbox::Notify(void *priv, const uint32_t id) {
    uint32_t channel;

    switch(id){
        case 0/* VRING0_ID */:
            // new message to process for A7
            channel = IPCC_CHANNEL_1;
            break;

        case 1/* VRING1_ID */:
            // (XXX: from ST code) Note: the OpenAMP framework never notifies this
            // buffer free (we processed the last message from A7)
            channel = IPCC_CHANNEL_2;
            break;

        default:
            Logger::Error("%s: %s (%u)", __FUNCTION__, "invalid vring id", id);
            return -1;
    }

    // check that channel is available (waiting if needed)
    if(HAL_IPCC_GetChannelStatus(&gHandle, channel, IPCC_CHANNEL_DIR_TX) == IPCC_CHANNEL_STATUS_OCCUPIED) {
        Logger::Trace("Waiting for channel %d free (vring id %u)", channel, id);
        while(HAL_IPCC_GetChannelStatus(&gHandle, channel, IPCC_CHANNEL_DIR_TX) == IPCC_CHANNEL_STATUS_OCCUPIED) {
            // wait
        }
    }

    // notify host
    HAL_IPCC_NotifyCPU(&gHandle, channel, IPCC_CHANNEL_DIR_TX);

    return 0;
}



/**
 * @brief Acknowledge a shutdown request
 *
 * Notify the host that we've finished processing a shutdown request; we'll get turned off
 * immediately after.
 *
 * @remark Even if we do not send this acknowledgement, we'll get turned off within half a second
 *         of receiving the shutdown request.
 */
void Mailbox::AckShutdownRequest() {
    // acknowledge reception
    HAL_IPCC_NotifyCPU(&gHandle, IPCC_CHANNEL_3, IPCC_CHANNEL_DIR_RX);
}



/**
 * @brief IPCC receive interrupt handler
 */
extern "C" void IPCC_RX1_IRQHandler() {
    HAL_IPCC_RX_IRQHandler(&Mailbox::gHandle);
}

/**
 * @brief IPCC transmit interrupt handler
 */
extern "C" void IPCC_TX1_IRQHandler() {
    HAL_IPCC_TX_IRQHandler(&Mailbox::gHandle);
}
