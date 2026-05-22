#pragma once
// EngineAPI.h — сервисы движка доступные сцене.
//
// Jolt-синглтоны (аллокаторы, Factory) живут в RKengCore / exe.
// DLL-сцена вызывает InitJoltFromEngine(api) в начале OnLoad()
// чтобы пробросить их в свою копию libJolt.a.
// После этого любой Jolt-код в DLL работает нормально.
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

    struct EngineAPI
    {
        // ── Логгер ───────────────────────────────────────────────────────────
        void (*LogInfo )(const char* msg) = nullptr;
        void (*LogWarn )(const char* msg) = nullptr;
        void (*LogError)(const char* msg) = nullptr;

        // ── Физика: статические тела ─────────────────────────────────────────
        uint32_t (*SpawnStaticBox)(PhysicsState& ph, const RK_BoxBody& box) = nullptr;
        uint32_t (*SpawnStaticBoxRot)(PhysicsState& ph, const RK_StaticBox& box) = nullptr;

        // ── Физика: динамические тела ────────────────────────────────────────
        uint32_t (*SpawnDynamicBox)(PhysicsState& ph, const RK_DynamicBox& box) = nullptr;
        void     (*DestroyBody)(PhysicsState& ph, uint32_t bodyID) = nullptr;

        // ── Jolt синглтоны — для InitJoltFromEngine() ────────────────────────
        // DLL линкует свою копию libJolt.a с nullptr-синглтонами.
        // Движок передаёт реальные указатели здесь; DLL вызывает
        // InitJoltFromEngine(api) в самом начале OnLoad() чтобы их проставить.
        void* joltAllocate   = nullptr;   // JPH::AllocateFunction
        void* joltFree       = nullptr;   // JPH::FreeFunction
        void* joltReallocate = nullptr;   // JPH::ReallocateFunction
        void* joltAllocate16 = nullptr;   // JPH::AlignedAllocateFunction
        void* joltFree16     = nullptr;   // JPH::AlignedFreeFunction
        void* joltFactory    = nullptr;   // JPH::Factory*
        void* joltAssertFn   = nullptr;   // JPH::AssertFailedFunction

        // ── Утилиты ──────────────────────────────────────────────────────────
        uint32_t engineVersion = 0;
    };
}
