#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include "stm32mp1xx.h"

#include <openamp/open_amp.h>
#include <stdarg.h>

#include "OpenAmp.h"
#include "Mailbox.h"
#include "MessageHandler.h"
#include "Rpc.h"

using namespace Rpc;

static MessageHandler *gTask{nullptr};

/**
 * Set up the hardware required by the RPC communications (namely, IPCC) and then start the task
 * that handles processing messages via OpenAMP.
 */
void Rpc::Init() {
    REQUIRE(!gTask, "cannot re-initialize RPC");

    Mailbox::Init();
    OpenAmp::Init();

    gTask = new MessageHandler;
}

/**
 * @brief Get the global message handler instance
 *
 * Return the message handler task, which serves as the authority for registered message
 * endpoints and allows communication over said endpoints. It's needed by any task that wants to
 * expose an RPC endpoint later.
 */
MessageHandler *Rpc::GetHandler() {
    return gTask;
}
