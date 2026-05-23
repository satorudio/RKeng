#include "ServerWorldTick.h"
#include "../network/PacketSend.h"
#include "../network/PacketDelta.h"
#include "../network/Protocol.h"
#include <cmath>
#include <vector>
#include <cstring>
#include <unordered_map>
#include <algorithm>

namespace RKeng::Server::ServerWorldTick
{
    static constexpr float WALK_SPEED   = 5.0f;
    static constexpr float RUN_SPEED    = 10.0f;
    static constexpr float CROUCH_SPEED = 2.5f;
    static constexpr float JUMP_VEL     = 8.0f;
    static constexpr float GRAVITY      = -20.0f;
    static constexpr float FLOOR_Y      = 0.0f;

    static void SimulatePlayer(ConnectedPlayer& p, float dt)
    {
        using namespace Protocol;
        uint8_t btn = p.lastInput.buttons;
        float   yaw = p.yaw;

        float sinY = std::sin(yaw), cosY = std::cos(yaw);
        float dx = 0, dz = 0;
        if (btn & InputBits::FORWARD)  { dx -= sinY; dz -= cosY; }
        if (btn & InputBits::BACKWARD) { dx += sinY; dz += cosY; }
        if (btn & InputBits::RIGHT)    { dx += cosY; dz -= sinY; }
        if (btn & InputBits::LEFT)     { dx -= cosY; dz += sinY; }

        float len = std::sqrt(dx*dx + dz*dz);
        if (len > 1e-4f) { dx /= len; dz /= len; }

        float speed = (btn & InputBits::CROUCH) ? CROUCH_SPEED
                    : (btn & InputBits::RUN)    ? RUN_SPEED
                    :                             WALK_SPEED;
        p.x += dx * speed * dt;
        p.z += dz * speed * dt;

        bool onGround = (p.y <= FLOOR_Y + 0.01f);
        if (onGround && (btn & InputBits::JUMP)) p.velY = JUMP_VEL;
        if (!onGround) p.velY += GRAVITY * dt;
        else if (p.velY < 0.0f) p.velY = 0.0f;

        p.y += p.velY * dt;
        if (p.y < FLOOR_Y) { p.y = FLOOR_Y; p.velY = 0.0f; }

        p.x = std::clamp(p.x, -24.0f, 24.0f);
        p.z = std::clamp(p.z, -24.0f, 24.0f);
    }

    void Run(ServerState& srv, float deltaTime)
    {
        for (auto& [id, player] : srv.players)
            SimulatePlayer(player, deltaTime);

        srv.serverTick++;
        if (srv.serverTick % 2 != 0) return;

        std::vector<Protocol::PlayerSnapshot> snapshots;
        snapshots.reserve(srv.players.size());
        for (auto& [id, p] : srv.players)
        {
            Protocol::PlayerSnapshot snap{};
            snap.playerID = p.id;
            snap.x = p.x; snap.y = p.y; snap.z = p.z;
            snap.yaw = p.yaw;
            snapshots.push_back(snap);
        }

        static std::unordered_map<uint32_t, Protocol::PlayerSnapshot> s_prevSnaps;

        std::vector<uint8_t> payload;
        payload.reserve(256);
        payload.resize(5);
        memcpy(payload.data(), &srv.serverTick, 4);
        payload[4] = static_cast<uint8_t>(snapshots.size());

        uint8_t deltaBuf[Protocol::MAX_DELTA_PACKET];
        for (auto& snap : snapshots)
        {
            auto it = s_prevSnaps.find(snap.playerID);
            const Protocol::PlayerSnapshot* prev = (it != s_prevSnaps.end()) ? &it->second : nullptr;
            size_t sz = Protocol::EncodeDeltaSnapshot(prev, snap, deltaBuf);

            size_t offset = payload.size();
            payload.resize(offset + 1 + sz);
            payload[offset] = static_cast<uint8_t>(sz);
            memcpy(payload.data() + offset + 1, deltaBuf, sz);
            s_prevSnaps[snap.playerID] = snap;
        }

        std::vector<uint8_t> buf(sizeof(Protocol::Header) + payload.size());
        auto* hdr = reinterpret_cast<Protocol::Header*>(buf.data());
        hdr->type        = Protocol::PacketType::S_WorldState;
        hdr->payloadSize = static_cast<uint16_t>(payload.size());
        memcpy(buf.data() + sizeof(Protocol::Header), payload.data(), payload.size());

        PacketSend::Broadcast(srv.sock, srv.players, buf.data(), buf.size());
    }
}
