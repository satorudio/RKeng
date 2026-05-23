#include "ServerShutdown.h"
#include <cstdio>

namespace RKeng::Server::ServerShutdown
{
    void Run(ServerState& srv)
    {
        if (srv.sock != RK_INVALID_SOCK)
        {
            Net::Close(srv.sock);
            srv.sock = RK_INVALID_SOCK;
        }
        Net::Shutdown();
        srv.running = false;
        printf("[Server] Shutdown complete.\n");
    }
}
