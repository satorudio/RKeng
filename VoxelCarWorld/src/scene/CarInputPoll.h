#pragma once
// CarInputPoll.h — GLFW-опрос управления RAM 2500 Power Wagon.

#include "scene/CarState.h"
#include "SceneState.h"

namespace RKeng::CarInputPoll
{
    // Опрашивает GLFW (через glfwGetCurrentContext), заполняет car.input и camYaw/pitch.
    // dt — для плавного нарастания throttle/brake/steer.
    void Run(CarState& car, const SceneState& scene, float dt);
}
