#pragma once
// EngineAPI.h — сервисы движка доступные сцене.
#include <cstdint>

namespace RKeng
{
    struct PhysicsState;

    struct RK_Vec3  { float x, y, z; };
    struct RK_Color { float r, g, b; };

    struct RK_StaticBox
    {
        float cx, cy, cz;
        float hx, hy, hz;
        float rotY = 0.0f;
        float rotX = 0.0f;
    };

    struct RK_DynamicBox
    {
        float cx, cy, cz;
        float hx, hy, hz;
        float mass           = 1200.0f;
        float linearDamping  = 0.05f;
        float angularDamping = 0.4f;
        float friction       = 0.3f;
    };

    struct RK_BoxBody
    {
        RK_Vec3  position;
        RK_Vec3  halfExtents;
        bool     isSensor = false;
    };

    // Описание капсулы персонажа для CreateCharacter()
    struct RK_CharacterDesc
    {
        float spawnX = 0.f, spawnY = 2.f, spawnZ = 0.f;
        float capsuleHalfHeight = 0.9f;   // высота цилиндрической части капсулы
        float capsuleRadius     = 0.35f;  // радиус капсулы
        float maxSlopeAngleDeg  = 45.f;   // макс угол подъёма
    };

    struct EngineAPI
    {
        // ── Логгер ───────────────────────────────────────────────────────────
        void (*LogInfo )(const char* msg) = nullptr;
        void (*LogWarn )(const char* msg) = nullptr;
        void (*LogError)(const char* msg) = nullptr;

        // ── Физика: статические тела ─────────────────────────────────────────
        uint32_t (*SpawnStaticBox)   (PhysicsState& ph, const RK_BoxBody& box)   = nullptr;
        uint32_t (*SpawnStaticBoxRot)(PhysicsState& ph, const RK_StaticBox& box) = nullptr;

        // ── Физика: динамические тела ────────────────────────────────────────
        uint32_t (*SpawnDynamicBox)(PhysicsState& ph, const RK_DynamicBox& box) = nullptr;
        void     (*DestroyBody)   (PhysicsState& ph, uint32_t bodyID)           = nullptr;

        // ── Физика: персонаж ─────────────────────────────────────────────────
        // Создаёт CharacterVirtual в движке. Вызывать ПОСЛЕ SpawnStaticBox +
        // OptimizeBroadPhase. CharacterVirtual.h не включается в DLL-плагине
        // (JPH_IMPLEMENT_RTTI_VIRTUAL → краш при LoadLibraryA).
        bool     (*CreateCharacter)(PhysicsState& ph, const RK_CharacterDesc& desc) = nullptr;

        bool     (*GetBodyTransform)(PhysicsState& ph, uint32_t bodyID,
                     float& px, float& py, float& pz,
                     float& qx, float& qy, float& qz, float& qw) = nullptr;

        void     (*SetPlayerVelocity)(PhysicsState& ph, float vx, float vy, float vz) = nullptr;
        void     (*GetPlayerVelocity)(PhysicsState& ph, float& vx, float& vy, float& vz) = nullptr;
        float    (*GetGravityY)      (PhysicsState& ph)                                   = nullptr;

        // ── Версия ───────────────────────────────────────────────────────────
        // 3 — базовый набор (GetBodyTransform, SetPlayerVelocity, ...)
        // 4 — добавлен CreateCharacter
        // 5 — добавлены Jolt-синглтоны для InitJoltFromEngine() в DLL
        uint32_t engineVersion = 0;

        // ── Jolt синглтоны ───────────────────────────────────────────────────
        // Передаются движком в DLL-сцену чтобы та могла вызвать
        // InitJoltFromEngine() и использовать единственный инстанс Jolt.
        // Поля заполняются только если движок собран с RK_JOLT_ENABLED.
        void* joltAllocate   = nullptr;  // JPH::Allocate
        void* joltFree       = nullptr;  // JPH::Free
        void* joltReallocate = nullptr;  // JPH::Reallocate
        void* joltAllocate16 = nullptr;  // JPH::AlignedAllocate
        void* joltFree16     = nullptr;  // JPH::AlignedFree
        void* joltFactory    = nullptr;  // JPH::Factory::sInstance
        void* joltAssertFn   = nullptr;  // JPH::AssertFailed
    };
}
