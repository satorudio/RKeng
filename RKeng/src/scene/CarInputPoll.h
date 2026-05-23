#pragma once
#include "CarState.h"
#include "../core/SceneState.h"

namespace RKeng::CarInputPoll
{
    // Вызывать каждый кадр после InputPoll::Run
    void Run(CarState& car, const SceneState& scene, float dt);
}
