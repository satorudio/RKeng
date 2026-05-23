#pragma once
#include "RKExport.h"
#include "CarState.h"
#include "../core/SceneState.h"

namespace RKeng::CarInputPoll
{
    RK_API void Run(CarState& car, const SceneState& scene, float dt);
}
