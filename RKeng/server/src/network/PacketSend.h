#pragma once
#include "Protocol.h"
#include "../network/RKSocket.h"
#include "../core/ServerState.h"
#include <cstddef>
#include <unordered_map>

namespace RKeng::Server::PacketSend
{
    void Send(RKSock sock, Net::Addr to, const void* data, size_t size);
    void Broadcast(RKSock sock,
                   const std::unordered_map<uint32_t, struct ConnectedPlayer>& players,
                   const void* data, size_t size);
}
