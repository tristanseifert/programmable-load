#ifndef RPC_RPC_H
#define RPC_RPC_H

namespace Rpc::Confd {
class Service;
}

/**
 * @brief Remote procedure call interface to host
 */
namespace Rpc {
class MessageHandler;

using ConfdService = Rpc::Confd::Service;

/// Initialize the RPC system, including OpenAMP
void Init();
/// Get the global message handler instance
MessageHandler *GetHandler();

/// Get the configuration service interface
ConfdService *GetConfigService();
}

#endif
