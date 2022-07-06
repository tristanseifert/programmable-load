#ifndef RPC_RPC_H
#define RPC_RPC_H

/**
 * @brief Remote procedure call interface to host
 */
namespace Rpc {
class MessageHandler;

/// Initialize the RPC system, including OpenAMP
void Init();
/// Get the global message handler instance
MessageHandler *GetHandler();
}

#endif
