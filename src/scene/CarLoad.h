#pragma once
#include "CarState.h"
#include "../physics/PhysicsState.h"

namespace RKeng::CarLoad
{
    // Создаёт физическое тело машины с VehicleConstraint
    void Run(CarState& car, PhysicsState& ph, Vec3 spawnPos = {0.0f, 1.0f, 0.0f});

    // Освободить тело и constraint
    void Destroy(CarState& car, PhysicsState& ph);
}
