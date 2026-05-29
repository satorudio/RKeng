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
        float capsuleHalfHeight = 0.9f;
        float capsuleRadius     = 0.35f;
        float maxSlopeAngleDeg  = 45.f;
    };

    // ── Транспортное средство (версия API 6) ────────────────────────────────
    // Все параметры — plain data, никакого Jolt в DLL.
    // Движок создаёт VehicleConstraint внутри себя и возвращает хэндл.
    struct RK_VehicleDesc
    {
        // Кузов
        float spawnX = 0.f, spawnY = 2.f, spawnZ = 0.f;
        float halfW  = 1.0f,  halfH  = 0.4f,  halfL = 2.5f;
        float mass   = 1500.f;
        float linearDamping  = 0.05f;
        float angularDamping = 0.5f;
        float bodyFriction   = 0.3f;

        // Подвеска (одинакова для всех 4 колёс)
        float suspMinLen  = 0.05f;
        float suspMaxLen  = 0.25f;
        float suspFreq    = 2.0f;
        float suspDamping = 0.5f;

        // Колёса
        float wheelRadius   = 0.36f;
        float wheelWidth    = 0.15f;
        float maxSteerDeg   = 30.0f;

        // Двигатель
        float maxTorque     = 500.0f;
        float maxRPM        = 6000.0f;
        float engineInertia = 0.5f;

        // Антикрен
        float antiRollFront = 1000.0f;
        float antiRollRear  = 1000.0f;

        // Фрикция колёс
        float frontFriction = 1.6f;
        float rearFriction  = 1.6f;
    };

    // Хэндл на созданное транспортное средство.
    // Непрозрачен для DLL — передаётся обратно в API-функции.
    using RK_VehicleHandle = uint32_t;
    static constexpr RK_VehicleHandle RK_INVALID_VEHICLE = 0xFFFFFFFFu;

    // Инпут для SetVehicleInput()
    struct RK_VehicleInput
    {
        float throttle  = 0.f;  // 0..1
        float brake     = 0.f;  // 0..1
        float steer     = 0.f;  // -1..1 (лево отрицательное)
        float handbrake = 0.f;  // 0 или 1
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
        bool     (*CreateCharacter)(PhysicsState& ph, const RK_CharacterDesc& desc) = nullptr;

        bool     (*GetBodyTransform)(PhysicsState& ph, uint32_t bodyID,
                     float& px, float& py, float& pz,
                     float& qx, float& qy, float& qz, float& qw) = nullptr;

        void     (*SetPlayerVelocity)(PhysicsState& ph, float vx, float vy, float vz) = nullptr;
        void     (*GetPlayerVelocity)(PhysicsState& ph, float& vx, float& vy, float& vz) = nullptr;
        float    (*GetGravityY)      (PhysicsState& ph)                                   = nullptr;

        // ── Транспорт (версия API 6) ─────────────────────────────────────────
        // VehicleConstraint создаётся и живёт внутри движка.
        // DLL не линкует Jolt, не включает VehicleConstraint.h.

        // Создать машину + зарегистрировать в PhysicsSystem (AddConstraint+AddStepListener).
        // Возвращает RK_INVALID_VEHICLE при ошибке.
        RK_VehicleHandle (*SpawnVehicle)(PhysicsState& ph, const RK_VehicleDesc& desc) = nullptr;

        // Установить инпут водителя (вызывать каждый тик до шага физики).
        void (*SetVehicleInput)(PhysicsState& ph, RK_VehicleHandle vh,
                                const RK_VehicleInput& inp) = nullptr;

        // Получить трансформ кузова (позиция + кватернион) и скорость.
        bool (*GetVehicleTransform)(PhysicsState& ph, RK_VehicleHandle vh,
                                    float& px, float& py, float& pz,
                                    float& qx, float& qy, float& qz, float& qw,
                                    float& vx, float& vy, float& vz) = nullptr;

        // Удалить машину (RemoveStepListener + RemoveConstraint + DestroyBody + освободить слот).
        void (*DestroyVehicle)(PhysicsState& ph, RK_VehicleHandle vh) = nullptr;

        // Оптимизировать BVH после добавления всех статических тел.
        // ОБЯЗАТЕЛЬНО вызывать после WorldGen::Generate, иначе raycast колёс не найдёт пол.
        void (*OptimizeBroadPhase)(PhysicsState& ph) = nullptr;

        // ── Версия ───────────────────────────────────────────────────────────
        // 3 — базовый набор
        // 4 — CreateCharacter
        // 5 — Jolt-синглтоны (joltFactory, joltAllocate, ...)
        // 6 — SpawnVehicle / SetVehicleInput / GetVehicleTransform / DestroyVehicle
        // 7 — OptimizeBroadPhase
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
