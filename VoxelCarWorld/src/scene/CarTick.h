#pragma once
// CarTick.h — физический тик машины, грязь, дебрис.

#include "CarState.h"
#include "PhysicsState.h"
#include "SceneState.h"
#include "WorldGen.h"

namespace RKeng::CarTick
{
    // Основной тик — вызывается из OnTick каждый кадр.
    // mudZones — список зон грязи из WorldData (передаётся раз при загрузке).
    void Run(CarState& car, PhysicsState& ph, SceneState& scene,
             float dt,
             const std::vector<WorldGen::MudZone>& mudZones = {});
}
