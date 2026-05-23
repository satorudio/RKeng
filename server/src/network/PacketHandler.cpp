#include "PacketHandler.h"
#include "PacketSend.h"
#include <cstdio>
#include <cstring>

namespace RKeng::Server::PacketHandler
{
    static void HandleHello(ServerState& srv, Net::Addr from,
                            const void* data, size_t size)
    {
        if (size < sizeof(Protocol::C_Hello)) return;
        auto* pkt = reinterpret_cast<const Protocol::C_Hello*>(data);

        if (pkt->version != Protocol::VERSION)
        {
            Protocol::S_Reject rej;
            snprintf(rej.reason, sizeof(rej.reason),
                     "Wrong protocol version. Server=%u Client=%u",
                     Protocol::VERSION, pkt->version);
            PacketSend::Send(srv.sock, from, &rej, sizeof(rej));
            return;
        }

        // Уже подключён с этого адреса?
        uint64_t key = AddrKey(from.ip, from.port);
        if (srv.addrToID.count(key)) return;

        uint32_t id = srv.nextPlayerID++;
        ConnectedPlayer player;
        player.id        = id;
        player.addr      = from;
        player.connected = true;
        strncpy(player.name, pkt->name, 31);
        player.name[31]  = '\0';

        srv.players[id]  = player;
        srv.addrToID[key] = id;

        Protocol::S_Welcome welcome;
        welcome.playerID = id;
        welcome.spawnX   = 0.0f;
        welcome.spawnY   = 2.0f;
        welcome.spawnZ   = 0.0f;
        PacketSend::Send(srv.sock, from, &welcome, sizeof(welcome));

        printf("[Server] Player '%s' joined (id=%u). Total: %zu\n",
               player.name, id, srv.players.size());
    }

    static void HandleInputState(ServerState& srv, Net::Addr from,
                                 const void* data, size_t size)
    {
        if (size < sizeof(Protocol::C_InputState)) return;
        uint64_t key = AddrKey(from.ip, from.port);
        auto it = srv.addrToID.find(key);
        if (it == srv.addrToID.end()) return;

        auto pit = srv.players.find(it->second);
        if (pit == srv.players.end()) return;

        auto* pkt = reinterpret_cast<const Protocol::C_InputState*>(data);
        pit->second.lastInput     = *pkt;
        pit->second.lastInputTick = pkt->tick;
        pit->second.yaw           = pkt->yaw;
        pit->second.pitch         = pkt->pitch;
    }

    static void HandleDisconnect(ServerState& srv, Net::Addr from)
    {
        uint64_t key = AddrKey(from.ip, from.port);
        auto it = srv.addrToID.find(key);
        if (it == srv.addrToID.end()) return;

        auto pit = srv.players.find(it->second);
        if (pit != srv.players.end())
        {
            printf("[Server] Player '%s' (id=%u) disconnected.\n",
                   pit->second.name, pit->second.id);
            srv.players.erase(pit);
        }
        srv.addrToID.erase(it);
    }

    void Dispatch(ServerState& srv, Net::Addr from,
                  Protocol::Header* hdr, const void* data, size_t size)
    {
        switch (hdr->type)
        {
        case Protocol::PacketType::C_Hello:
            HandleHello(srv, from, data, size);
            break;
        case Protocol::PacketType::C_InputState:
            HandleInputState(srv, from, data, size);
            break;
        case Protocol::PacketType::C_Disconnect:
            HandleDisconnect(srv, from);
            break;
        default:
            printf("[Server] Unknown packet type: 0x%02x\n",
                   static_cast<uint8_t>(hdr->type));
            break;
        }
    }
}
