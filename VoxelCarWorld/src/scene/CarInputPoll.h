#pragma once
#include "CarState.h"
#include "SceneState.h"

namespace RKeng::CarInputPoll
{
    // Опрашивает GLFW через scene.windowHandle.
    // Заполняет car.input, car.camYaw, car.camPitch, car.camDist.
    void Run(CarState& car, const SceneState& scene, float dt);
}
