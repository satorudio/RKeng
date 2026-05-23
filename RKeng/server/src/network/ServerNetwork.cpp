#include "ServerNetwork.h"
#include "PacketHandler.h"
#include <cstdio>

namespace RKeng::Server::ServerNetwork
{
    static constexpr int MAX_PACKET = 1400; // безопасный UDP MTU

    void PollEvents(ServerState& srv)
    {
        uint8_t buf[MAX_PACKET];
        Net::Addr from{};

        // Читаем все доступные пакеты за один тик (сокет неблокирующий)
        for (;;)
        {
            int r = Net::RecvFrom(srv.sock, buf, MAX_PACKET, from);
            if (r <= 0) break; // 0 = нет данных, -1 = ошибка

            if (r < static_cast<int>(sizeof(Protocol::Header))) continue;

            auto* hdr = reinterpret_cast<Protocol::Header*>(buf);
            PacketHandler::Dispatch(srv, from, hdr, buf, static_cast<size_t>(r));
        }
    }
}
