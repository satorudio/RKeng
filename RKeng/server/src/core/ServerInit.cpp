#include "ServerInit.h"
#include <cstdio>
#include <stdexcept>

namespace RKeng::Server::ServerInit
{
    void Run(ServerState& srv)
    {
        if (!Net::Init())
            throw std::runtime_error("[Server] Net::Init failed (WSAStartup?).");

        srv.sock = Net::CreateUDP(static_cast<uint16_t>(srv.port));
        if (srv.sock == RK_INVALID_SOCK)
            throw std::runtime_error("[Server] CreateUDP failed — порт занят?");

        srv.running = true;
        printf("[Server] UDP listening on port %u (max %u players)\n",
               srv.port, srv.maxPlayers);
    }
}
