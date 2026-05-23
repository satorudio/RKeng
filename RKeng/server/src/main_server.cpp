#include "core/ServerState.h"
#include "core/ServerInit.h"
#include "core/ServerShutdown.h"
#include "network/ServerNetwork.h"
#include "world/ServerWorldTick.h"

#include <cstdio>
#include <chrono>
#include <thread>

// main_server.cpp — чистый дирижёр сервера.
// Запускается отдельным процессом рядом с клиентом.
// Никакой игровой логики здесь — только порядок вызовов.

int main()
{
    using namespace RKeng::Server;
    using Clock = std::chrono::steady_clock;

    auto& srv = Get();
    srv.port       = 7777;
    srv.maxPlayers = 64;

    try { ServerInit::Run(srv); }
    catch (const std::exception& e)
    {
        printf("[Server] FATAL: %s\n", e.what());
        return 1;
    }

    printf("[Server] Running. Ctrl+C to stop.\n");

    constexpr float TARGET_DT = 1.0f / 60.0f;

    auto lastTime = Clock::now();

    while (srv.running)
    {
        auto now = Clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        ServerNetwork::PollEvents(srv);
        ServerWorldTick::Run(srv, dt);

        // Спим остаток тика чтобы не жрать 100% CPU
        auto tickEnd = lastTime + std::chrono::duration<float>(TARGET_DT);
        std::this_thread::sleep_until(tickEnd);
    }

    ServerShutdown::Run(srv);
    return 0;
}
