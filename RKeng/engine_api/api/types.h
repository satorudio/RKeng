#pragma once
#include <cstdint>

namespace RKeng
{
    // ── Непрозрачный хендл физического мира ─────────────────────────────────
    using RK_WorldHandle = uint32_t;
    static constexpr RK_WorldHandle RK_WORLD = 0u;

    // ── Базовые типы ─────────────────────────────────────────────────────────
    struct RK_Vec3  { float x, y, z; };
    struct RK_Color { float r, g, b; };

    // ── Физика: описания тел ─────────────────────────────────────────────────
    struct RK_BoxBody
    {
        RK_Vec3 position;
        RK_Vec3 halfExtents;
        bool    isSensor = false;
    };

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

    // ── Персонаж ─────────────────────────────────────────────────────────────
    struct RK_CharacterDesc
    {
        float spawnX = 0.f, spawnY = 2.f, spawnZ = 0.f;
        float capsuleHalfHeight = 0.9f;
        float capsuleRadius     = 0.35f;
        float maxSlopeAngleDeg  = 45.f;
    };

    // ── Транспорт ─────────────────────────────────────────────────────────────
    using RK_VehicleHandle = uint32_t;
    static constexpr RK_VehicleHandle RK_INVALID_VEHICLE = 0xFFFFFFFFu;

    struct RK_VehicleDesc
    {
        float spawnX = 0.f, spawnY = 2.f, spawnZ = 0.f;
        float halfW  = 1.0f, halfH = 0.4f, halfL = 2.5f;
        float mass   = 1500.f;
        float linearDamping  = 0.05f;
        float angularDamping = 0.5f;
        float bodyFriction   = 0.3f;
        float suspMinLen  = 0.05f;
        float suspMaxLen  = 0.25f;
        float suspFreq    = 2.0f;
        float suspDamping = 0.5f;
        float wheelRadius   = 0.36f;
        float wheelWidth    = 0.15f;
        float maxSteerDeg   = 30.0f;
        float maxTorque     = 500.0f;
        float maxRPM        = 6000.0f;
        float engineInertia = 0.5f;
        float antiRollFront = 1000.0f;
        float antiRollRear  = 1000.0f;
        float frontFriction = 1.6f;
        float rearFriction  = 1.6f;
    };

    struct RK_VehicleInput
    {
        float throttle  = 0.f;
        float brake     = 0.f;
        float steer     = 0.f;
        float handbrake = 0.f;
    };
}
