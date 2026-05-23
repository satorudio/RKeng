#pragma once
// CarConstraint.h — регистрация/разрегистрация VehicleConstraint в PhysicsSystem.
//
// Вынесено из OpenCarWorld.cpp чтобы не тащить туда VehicleConstraint.h.
// VehicleConstraint.h содержит JPH_IMPLEMENT_RTTI_VIRTUAL — глобальные
// RTTI-объекты которые инициализируются при загрузке DLL ДО DllMain,
// когда Factory::sInstance ещё nullptr → 0x80000003.
//
// В этом .cpp нет глобальных переменных с конструкторами — безопасно.

#include "PhysicsState.h"
#include "CarState.h"

namespace RKeng::CarConstraint
{
    // Добавить VehicleConstraint в PhysicsSystem (после CarLoad::Run)
    void Register(CarState& car, PhysicsState& ph);

    // Удалить VehicleConstraint из PhysicsSystem (перед CarLoad::Destroy)
    void Unregister(CarState& car, PhysicsState& ph);
}
