#include "Log/Logger.h"
#include "Rtos/Rtos.h"

#include "stm32mp1xx.h"

#include <openamp/open_amp.h>
#include <stdarg.h>

#include "OpenAmp.h"
#include "Mailbox.h"
#include "MessageHandler.h"
#include "Rpc.h"

#include "Endpoints/Confd/Handler.h"
#include "Endpoints/Confd/Service.h"
#include "Endpoints/ResourceManager/Handler.h"
#include "Endpoints/ResourceManager/Service.h"

using namespace Rpc;

static MessageHandler *gTask{nullptr};
static Confd::Service *gConfdService{nullptr};
static ResourceManager::Service *gResMgrService{nullptr};

/**
 * Set up the hardware required by the RPC communications (namely, IPCC) and then start the task
 * that handles processing messages via OpenAMP.
 */
void Rpc::Init() {
    REQUIRE(!gTask, "cannot re-initialize RPC");

    // initialize the OpenAMP framework and our message handling machinery
    Mailbox::Init();
    OpenAmp::Init();

    gTask = new MessageHandler;

    // set up the endpoints
    auto confdHandler = new Rpc::Confd::Handler;
    confdHandler->attach(gTask);

    gConfdService = new Rpc::Confd::Service(confdHandler);

    auto resmgrHandler = new Rpc::ResourceManager::Handler;
    resmgrHandler->attach(gTask);

    gResMgrService = new Rpc::ResourceManager::Service(resmgrHandler);
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

Rpc::Confd::Service *Rpc::GetConfigService() {
    return gConfdService;
}
Rpc::ResourceManager::Service *Rpc::GetResMgrService() {
    return gResMgrService;
}
