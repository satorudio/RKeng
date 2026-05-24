#pragma once
#include "RKExport.h"
#include "CarState.h"
#include "PhysicsState.h"
#include "SceneState.h"

namespace RKeng::CarTick
{
    RK_API void RegisterContactCallback(CarState& car, PhysicsState& ph);
    RK_API void Run(CarState& car, PhysicsState& ph, SceneState& scene, float dt);
}
