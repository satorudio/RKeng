#pragma once
#include "types.h"
#include <cstdint>

namespace RKeng {
    struct PhysicsAPI {
        // ── Статические тела ─────────────────────────────────────────────────
        uint32_t (*SpawnStaticBox)   (RK_WorldHandle world, const RK_BoxBody& box)   = nullptr;
        uint32_t (*SpawnStaticBoxRot)(RK_WorldHandle world, const RK_StaticBox& box) = nullptr;

        // ── Динамические тела ────────────────────────────────────────────────
        uint32_t (*SpawnDynamicBox)(RK_WorldHandle world, const RK_DynamicBox& box) = nullptr;
        void     (*DestroyBody)   (RK_WorldHandle world, uint32_t bodyID)           = nullptr;

        // ── Трансформ ────────────────────────────────────────────────────────
        bool (*GetBodyTransform)(RK_WorldHandle world, uint32_t bodyID,
                                 float& px, float& py, float& pz,
                                 float& qx, float& qy, float& qz, float& qw) = nullptr;

        // ── BVH-оптимизация ──────────────────────────────────────────────────
        // ОБЯЗАТЕЛЬНО вызывать после WorldGen::Generate, иначе raycast колёс не найдёт пол.
        void (*OptimizeBroadPhase)(RK_WorldHandle world) = nullptr;
    };
}
