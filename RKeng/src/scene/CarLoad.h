#pragma once
#include "RKExport.h"
#include "CarState.h"
#include "../physics/PhysicsState.h"

namespace RKeng::CarLoad
{
    RK_API void Run(CarState& car, PhysicsState& ph, Vec3 spawnPos = {0.0f, 1.0f, 0.0f});
    RK_API void Destroy(CarState& car, PhysicsState& ph);
}
