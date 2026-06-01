#pragma once
// EngineAPI.h — единственная точка входа публичного API движка.
// Сцена включает только этот файл. Jolt нигде не виден.

#include "api/types.h"
#include "api/log.h"
#include "api/physics.h"
#include "api/vehicle.h"
#include "api/character.h"

namespace RKeng
{
    struct EngineAPI : LogAPI, PhysicsAPI, VehicleAPI, CharacterAPI
    {
        // ── Версия ───────────────────────────────────────────────────────────
        // 3 — базовый набор
        // 4 — CreateCharacter
        // 5 — Jolt-синглтоны (joltFactory, joltAllocate, ...)
        // 6 — SpawnVehicle / SetVehicleInput / GetVehicleTransform / DestroyVehicle
        // 7 — OptimizeBroadPhase
        // 8 — RK_WorldHandle, модульный API (api/*.h)
        uint32_t engineVersion = 0;

        // ── Jolt синглтоны (версия 5, сохранены для совместимости) ──────────
        void* joltAllocate   = nullptr;
        void* joltFree       = nullptr;
        void* joltReallocate = nullptr;
        void* joltAllocate16 = nullptr;
        void* joltFree16     = nullptr;
        void* joltFactory    = nullptr;
        void* joltAssertFn   = nullptr;
        void* joltTrace      = nullptr;
    };
}
