// CarLoad.cpp — создаёт машину через api.SpawnVehicle().
// Никакого Jolt в DLL. Никакого InitJoltFromEngine.
// VehicleConstraint живёт в движке (EngineAPI_Impl.h).

#include "CarLoad.h"
#include "CarMesh.h"

namespace RKeng::CarLoad
{
    void Run(CarState& car, PhysicsState& ph, Vec3 spawnPos, const EngineAPI& api)
    {
        car.position    = spawnPos;
        car.orientation = Quat(1, 0, 0, 0);
        car.meshDirty   = true;

        if (!api.SpawnVehicle) {
            if (api.LogError) api.LogError("CarLoad: SpawnVehicle not available (engine too old?)");
            return;
        }

        const auto& p = car.params;

        RK_VehicleDesc desc{};
        desc.spawnX = spawnPos.x;
        desc.spawnY = spawnPos.y;
        desc.spawnZ = spawnPos.z;

        desc.halfW = p.halfW;
        desc.halfH = p.halfH;
        desc.halfL = p.halfL;
        desc.mass  = p.mass;

        desc.linearDamping  = p.linearDamping;
        desc.angularDamping = p.angularDamping;
        desc.bodyFriction   = p.friction;

        desc.suspMinLen  = p.suspMinLen;
        desc.suspMaxLen  = p.suspMaxLen;
        desc.suspFreq    = p.suspFreq;
        desc.suspDamping = p.suspDamping;

        desc.wheelRadius = p.wheelRadius;
        desc.wheelWidth  = p.wheelWidth;
        desc.maxSteerDeg = p.maxSteerDeg;

        desc.maxTorque     = p.maxTorque;
        desc.maxRPM        = p.maxRPM;
        desc.engineInertia = p.engineInertia;

        desc.antiRollFront = p.antiRollFront;
        desc.antiRollRear  = p.antiRollRear;

        desc.frontFriction = p.frontFriction;
        desc.rearFriction  = p.rearFriction;

        if (api.LogInfo) api.LogInfo("CarLoad: calling SpawnVehicle");
        car.vehicleHandle = api.SpawnVehicle(ph, desc); if (api.LogInfo) { auto s = "vehicleHandle=" + std::to_string(car.vehicleHandle); api.LogInfo(s.c_str()); }

        if (car.vehicleHandle == RK_INVALID_VEHICLE) {
            if (api.LogError) api.LogError("CarLoad: SpawnVehicle failed");
            return;
        }

        car.initialized = true;
        if (api.LogInfo) api.LogInfo("CarLoad: OK");
    }

    void Destroy(CarState& car, PhysicsState& ph, const EngineAPI& api)
    {
        if (!car.initialized) return;
        if (api.DestroyVehicle && car.vehicleHandle != RK_INVALID_VEHICLE) {
            api.DestroyVehicle(ph, car.vehicleHandle);
            car.vehicleHandle = RK_INVALID_VEHICLE;
        }
        car.initialized = false;
    }
}
