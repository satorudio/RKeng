#pragma once
#include "../core/ServerState.h"

// Одна задача: посчитать позиции всех игроков на сервере из их inputs.
// Сервер — единственная истина. Клиент только рисует.

namespace RKeng::Server::ServerWorldTick
{
    void Run(ServerState& srv, float deltaTime);
}
