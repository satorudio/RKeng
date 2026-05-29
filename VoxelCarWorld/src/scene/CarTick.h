#pragma once
#include "CarState.h"
#include "PhysicsState.h"
#include "SceneState.h"
#include "EngineAPI.h"

namespace RKeng::CarTick
{
    void Run(CarState& car, PhysicsState& ph, SceneState& scene,
             float dt, const EngineAPI& api);
}
