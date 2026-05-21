#pragma once
// EngineAPI.h — сервисы движка доступные сцене.
//
// ВСЕ функции создания физических тел живут в RKengCore.dll —
// там гарантированно инициализирован JPH::Factory::sInstance.
// DLL-плагин никогда не вызывает BoxShapeSettings::Create() напрямую.
#include <cstdint>

namespace RKeng
{
    struct PhysicsState;

    struct RK_Vec3  { float x, y, z; };
    struct RK_Color { float r, g, b; };

    // ── Статический бокс (пол, стены, трамплины) ─────────────────────────────
    struct RK_StaticBox
    {
        float cx, cy, cz;       // центр
        float hx, hy, hz;       // полуразмеры
        float rotY = 0.0f;      // yaw   (рад)
        float rotX = 0.0f;      // pitch (рад)
    };

    // ── Динамический бокс (кузов машины и т.п.) ──────────────────────────────
    struct RK_DynamicBox
    {
        float cx, cy, cz;           // начальная позиция
        float hx, hy, hz;           // полуразмеры
        float mass          = 1200.0f;
        float linearDamping = 0.05f;
        float angularDamping= 0.4f;
        float friction      = 0.3f;
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
        // Legacy (без ротации) — оставлен для совместимости
        uint32_t (*SpawnStaticBox)(PhysicsState& ph, const RK_BoxBody& box) = nullptr;

        // С полной ротацией — использовать везде вместо прямого Jolt
        uint32_t (*SpawnStaticBoxRot)(PhysicsState& ph,
                                      const RK_StaticBox& box) = nullptr;

        // ── Физика: динамические тела ────────────────────────────────────────
        // Создаёт Dynamic BoxBody (кузов машины).
        // Реализация в RKengCore.dll — Factory гарантированно инициализирован.
        // Возвращает BodyID::GetIndexAndSequenceNumber() или UINT32_MAX при ошибке.
        uint32_t (*SpawnDynamicBox)(PhysicsState& ph,
                                    const RK_DynamicBox& box) = nullptr;

        // Удалить тело из симуляции
        void (*DestroyBody)(PhysicsState& ph, uint32_t bodyID) = nullptr;

        // ── Утилиты ──────────────────────────────────────────────────────────
        uint32_t engineVersion = 0;
    };
}
