#pragma once
#include "RKExport.h"
#include "CarState.h"
#include "PhysicsState.h"
#include "EngineAPI.h"

namespace RKeng::CarLoad
{
    // api обязателен для создания физического тела через SpawnDynamicBox.
    RK_API void Run(CarState& car, PhysicsState& ph,
                    Vec3 spawnPos = {0.0f, 1.0f, 0.0f},
                    const EngineAPI* api = nullptr);
    RK_API void Destroy(CarState& car, PhysicsState& ph);
}
