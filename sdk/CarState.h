#pragma once
#include "MathTypes.h"
#include <array>
#include <cstdint>

#ifdef RK_JOLT_ENABLED
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#include <Jolt/Physics/Vehicle/WheeledVehicleController.h>
#endif

namespace RKeng
{
    // ----------------------------------------------------------------
    //  Воксельный меш машины
    //  Машина = 5x3x10 вокселей (ширина x высота x длина)
    // ----------------------------------------------------------------
    constexpr int   CAR_VOXELS_W = 5;
    constexpr int   CAR_VOXELS_H = 3;
    constexpr int   CAR_VOXELS_L = 10;
    constexpr float CAR_VOXEL_SIZE = 0.2f;

    struct CarVoxel
    {
        bool  alive   = true;
        Vec3  color   { 1.0f, 0.1f, 0.1f };  // красная машина
        float health  = 1.0f;                 // 0..1, при 0 — сносится
    };

    // Падающий осколок машины
    struct CarDebris
    {
        Vec3  pos;
        Vec3  velocity;
        Vec3  angularVel;
        Vec3  color;
        float size;
        float lifetime = 0.0f;
        bool  dead     = false;
    };

    // ----------------------------------------------------------------
    //  Входы управления
    // ----------------------------------------------------------------
    struct CarInput
    {
        float throttle    = 0.0f;   // 0..1  (W)
        float brake       = 0.0f;   // 0..1  (S)
        float steer       = 0.0f;   // -1..1 (A/D)
        bool  handbrake   = false;  // Shift
    };

    // ----------------------------------------------------------------
    //  Физические параметры машины (все в Jolt-единицах = метры)
    // ----------------------------------------------------------------
    struct CarPhysicsParams
    {
        // Кузов
        float halfExtentX   = CAR_VOXELS_W * CAR_VOXEL_SIZE * 0.5f;   // ~0.5
        float halfExtentY   = 0.2f;
        float halfExtentZ   = CAR_VOXELS_L * CAR_VOXEL_SIZE * 0.5f;   // ~1.0
        float mass          = 1200.0f;                                  // кг

        // Подвеска (каждое колесо)
        float suspensionMinLen      = 0.1f;
        float suspensionMaxLen      = 0.45f;
        float suspensionFrequency   = 2.5f;   // Hz — меньше = мягче
        float suspensionDamping     = 0.5f;

        // Колёса
        float wheelRadius   = 0.28f;
        float wheelWidth    = 0.16f;
        float maxSteerAngle = 30.0f;          // градусы

        // Двигатель
        float maxTorque         = 800.0f;     // Нм
        float maxRPM            = 6000.0f;
        float engineInertia     = 0.5f;

        // Трансмиссия (простая — прямой привод)
        float gearRatio         = 4.5f;
        float handbrakeForce    = 6000.0f;

        // Фрикция
        float frontFriction     = 1.5f;
        float rearFriction      = 1.5f;

        // Антиролл
        float antiRollFront     = 1000.0f;
        float antiRollRear      = 1000.0f;
    };

    // ----------------------------------------------------------------
    //  Damage / HP
    // ----------------------------------------------------------------
    struct CarDamage
    {
        float totalHP       = 100.0f;
        float currentHP     = 100.0f;
        bool  destroyed     = false;

        // При столкновении: импульс > этого порога -> срываем вокселы
        float voxelBreakImpulse = 800.0f;   // Н·с
        float voxelCrackImpulse = 300.0f;   // Н·с — только здоровье -=
    };

    // ----------------------------------------------------------------
    //  Главный стейт машины
    // ----------------------------------------------------------------
    struct CarState
    {
        // Управление
        CarInput   input;
        CarPhysicsParams params;
        CarDamage  damage;

        // Вокселы кузова
        CarVoxel voxels[CAR_VOXELS_W][CAR_VOXELS_H][CAR_VOXELS_L];
        bool meshDirty = true;

        // Отлетевшие куски
        std::vector<CarDebris> debris;

        // Текущая позиция/ориентация (для рендера)
        Vec3 position    { 0.0f, 1.0f, 0.0f };
        Quat orientation { 1,0,0,0 };
        Vec3 velocity    { 0,0,0 };
        float speedKph   = 0.0f;

        // Камера от первого лица
        Vec3 camLocalOffset  { 0.0f, CAR_VOXELS_H * CAR_VOXEL_SIZE + 0.1f, -CAR_VOXELS_L * CAR_VOXEL_SIZE * 0.3f };
        float camYaw         = 0.0f;
        float camPitch       = 0.0f;

        // GPU меш
        std::vector<float>    meshVertices;   // pos[3] + color[3] + normal[3]
        std::vector<uint32_t> meshIndices;

#ifdef RK_JOLT_ENABLED
        JPH::BodyID           bodyID;
        JPH::Ref<JPH::VehicleConstraint> vehicleConstraint;
#endif

        bool initialized = false;

        // Для детекции удара через deltaV — сохраняем скорость предыдущего кадра.
        // Не static в функции, чтобы корректно работало при нескольких машинах.
        float prevSpeed = 0.0f;
    };

    CarState& GetCarState();
}
