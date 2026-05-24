#pragma once
// CarState.h — стейт пикапа (box collider + VehicleConstraint).
// Без вокселей, без дебриса — просто машина.

#include "MathTypes.h"
#include <cstdint>

#ifdef RK_JOLT_ENABLED
#  include <Jolt/Jolt.h>
#  include <Jolt/Physics/Body/BodyID.h>
// VehicleConstraint.h и WheeledVehicleController.h — НЕ здесь:
// JPH_IMPLEMENT_RTTI_VIRTUAL → краш при LoadLibraryA до DllMain.
// Включаются только в .cpp после InitJoltFromEngine().
namespace JPH { class VehicleConstraint; }
#endif

namespace RKeng
{
    struct CarInput
    {
        float throttle  = 0.0f;   // 0..1   W
        float brake     = 0.0f;   // 0..1   S
        float steer     = 0.0f;   // -1..1  A/D
        bool  handbrake = false;  // Space
    };

    struct CarPhysicsParams
    {
        // Кузов (half-extents в метрах)
        float halfW  = 1.0f;
        float halfH  = 0.4f;
        float halfL  = 2.5f;
        float mass   = 1500.0f;

        float linearDamping  = 0.05f;
        float angularDamping = 0.5f;
        float friction       = 0.3f;

        // Подвеска
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

        // Тормоза
        float handbrakeForce = 5000.0f;

        // Антикрен
        float antiRollFront = 1000.0f;
        float antiRollRear  = 1000.0f;

        // Фрикция колёс
        float frontFriction = 1.6f;
        float rearFriction  = 1.6f;
    };

    struct CarState
    {
        CarInput         input;
        CarPhysicsParams params;

        // Трансформ (синхронизируется из Jolt каждый тик)
        Vec3  position    { 0.0f, 2.0f, 0.0f };
        Quat  orientation { 1.0f, 0.0f, 0.0f, 0.0f };
        Vec3  velocity    { 0.0f, 0.0f, 0.0f };
        float speedKph    = 0.0f;

        // Камера (третье лицо, вращается правой кнопкой)
        float camYaw   = 0.0f;
        float camPitch = -15.0f;
        float camDist  = 10.0f;

        // Меш машины (8 вершин box, строится один раз в CarLoad)
        std::vector<float>    meshVertices;  // pos[3]+color[3]+normal[3]
        std::vector<uint32_t> meshIndices;
        bool                  meshDirty = true;

#ifdef RK_JOLT_ENABLED
        JPH::BodyID             bodyID;
        JPH::VehicleConstraint* vehicleConstraint = nullptr;
#endif

        bool initialized = false;
    };
}
