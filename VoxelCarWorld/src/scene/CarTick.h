#pragma once
// CarTick.h — физический тик машины, грязь, дебрис.

#include "scene/CarState.h"
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

    // Регистрирует contact listener для детекции ударов кузова.
    // Вызывать после CarLoad::Run и CarConstraint::Register.
    // Заглушка — реализация добавляется по мере необходимости.
    void RegisterContactCallback(CarState& car, PhysicsState& ph);
}
