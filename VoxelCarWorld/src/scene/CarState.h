#pragma once
// CarState.h — стейт пикапа.
// Без Jolt-хедеров: vehicleHandle — непрозрачный uint32_t из движка.

#include "MathTypes.h"
#include "EngineAPI.h"  // RK_VehicleHandle, RK_INVALID_VEHICLE
#include <cstdint>
#include <vector>

namespace RKeng
{
    struct CarInput
    {
        float throttle  = 0.0f;
        float brake     = 0.0f;
        float steer     = 0.0f;
        bool  handbrake = false;
    };

    struct CarPhysicsParams
    {
        float halfW  = 1.0f;
        float halfH  = 0.4f;
        float halfL  = 2.5f;
        float mass   = 1500.0f;

        float linearDamping  = 0.05f;
        float angularDamping = 0.5f;
        float friction       = 0.3f;

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

        float handbrakeForce = 5000.0f;

        float antiRollFront = 1000.0f;
        float antiRollRear  = 1000.0f;

        float frontFriction = 1.6f;
        float rearFriction  = 1.6f;
    };

    struct CarState
    {
        CarInput         input;
        CarPhysicsParams params;

        Vec3  position    { 0.0f, 2.0f, 0.0f };
        Quat  orientation { 1.0f, 0.0f, 0.0f, 0.0f };
        Vec3  velocity    { 0.0f, 0.0f, 0.0f };
        float speedKph    = 0.0f;

        float camYaw   = 0.0f;   // смещение yaw относительно машины
        float camPitch = 0.0f;   // pitch взгляда

        std::vector<float>    meshVertices;
        std::vector<uint32_t> meshIndices;
        bool                  meshDirty = true;

        // Хэндл на VehicleConstraint внутри движка.
        // Непрозрачен — не Jolt-тип.
        RK_VehicleHandle vehicleHandle = RK_INVALID_VEHICLE;

        bool initialized = false;

        // Fly camera (debug mode, клавиша M)
        bool  flyCamActive = false;
        float flyCamX      = 0.0f;
        float flyCamY      = 10.0f;
        float flyCamZ      = 0.0f;
        float flyCamYaw    = 0.0f;
        float flyCamPitch  = 0.0f;
        float flyCamSpeed  = 20.0f;
        bool  mKeyWas      = false;  // для edge detection
    };
}
