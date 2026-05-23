#pragma once
#include "../network/Protocol.h"
#include "../network/RKSocket.h"
#include <cstdint>
#include <unordered_map>
#include <string>
#include <vector>

namespace RKeng::Server
{
    struct ConnectedPlayer
    {
        uint32_t  id       = 0;
        Net::Addr addr{};          // адрес клиента (ip+port)
        char      name[32] {};

        float x = 0, y = 2, z = 0;
        float yaw = 0, pitch = 0;
        float velY = 0.0f;

        Protocol::C_InputState lastInput{};
        uint32_t lastInputTick = 0;

        bool connected = false;
    };

    struct ServerState
    {
        RKSock    sock        = RK_INVALID_SOCK;
        uint32_t  port        = 7777;
        uint32_t  maxPlayers  = 64;

        uint32_t  nextPlayerID = 1;
        uint32_t  serverTick   = 0;

        std::unordered_map<uint32_t, ConnectedPlayer> players;  // id → player
        // Обратный маппинг ip:port → id (для быстрого поиска входящих пакетов)
        std::unordered_map<uint64_t, uint32_t> addrToID;

        bool running = false;
    };

    // Упаковать ip+port в uint64 для использования как ключа карты
    inline uint64_t AddrKey(uint32_t ip, uint16_t port)
    {
        return (static_cast<uint64_t>(ip) << 16) | port;
    }

    ServerState& Get();
}
