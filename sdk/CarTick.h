#pragma once
#include "CarState.h"
#include "PhysicsState.h"
#include "SceneState.h"

namespace RKeng::CarTick
{
    // Регистрирует коллбек на PhysicsState::contactListener.
    // Вызывать из CarLoad после создания car.bodyID.
    void RegisterContactCallback(CarState& car, PhysicsState& ph);

    void Run(CarState& car, PhysicsState& ph, SceneState& scene, float dt);
}
