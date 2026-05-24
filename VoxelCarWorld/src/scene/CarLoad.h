#pragma once
#include "CarState.h"
#include "PhysicsState.h"
#include "EngineAPI.h"

namespace RKeng::CarLoad
{
    void Run(CarState& car, PhysicsState& ph, Vec3 spawnPos, const EngineAPI& api);
    void Destroy(CarState& car, PhysicsState& ph);
}
