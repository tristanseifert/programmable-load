#ifndef RPC_RPC_H
#define RPC_RPC_H

namespace Rpc::Confd {
class Service;
}
namespace Rpc::ResourceManager {
class Service;
}

/**
 * @brief Remote procedure call interface to host
 */
namespace Rpc {
class MessageHandler;

using ConfdService = Rpc::Confd::Service;
using ResMgrService = Rpc::ResourceManager::Service;

/// Initialize the RPC system, including OpenAMP
void Init();
/// Get the global message handler instance
MessageHandler *GetHandler();

/// Get the configuration service interface
ConfdService *GetConfigService();
/// Get the resource manager interface
ResMgrService *GetResMgrService();
}

#endif
