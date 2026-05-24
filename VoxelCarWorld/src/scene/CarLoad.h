#pragma once
// CarLoad.h — создание физического тела RAM 2500 Power Wagon через EngineAPI.
//
// Кузов создаётся через api.SpawnDynamicBox (реализация в RKengCore.dll).
// VehicleConstraint/WheeledVehicleController создаются здесь — они не
// трогают JPH::Factory::sInstance, поэтому безопасны в DLL.

#include "scene/CarState.h"
#include "PhysicsState.h"
#include "EngineAPI.h"

namespace RKeng::CarLoad
{
    // Создаёт физику машины. api обязателен — через него создаётся кузов.
    void Run(CarState& car, PhysicsState& ph,
             Vec3 spawnPos = { 0.0f, 2.5f, 0.0f },
             const EngineAPI* api = nullptr);

    // Разрушить тело и VehicleConstraint перед выгрузкой сцены.
    void Destroy(CarState& car, PhysicsState& ph);
}
