#include "PacketSend.h"
#include "../core/ServerState.h"

namespace RKeng::Server::PacketSend
{
    void Send(RKSock sock, Net::Addr to, const void* data, size_t size)
    {
        Net::SendTo(sock, data, static_cast<int>(size), to.ip, to.port);
    }

    void Broadcast(RKSock sock,
                   const std::unordered_map<uint32_t, ConnectedPlayer>& players,
                   const void* data, size_t size)
    {
        for (auto& [id, p] : players)
            if (p.connected)
                Net::SendTo(sock, data, static_cast<int>(size), p.addr.ip, p.addr.port);
    }
}
