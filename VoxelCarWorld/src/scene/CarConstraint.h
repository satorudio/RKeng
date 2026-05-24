#pragma once
// CarConstraint.h — регистрация VehicleConstraint в PhysicsSystem.
// Вынесено в отдельный .cpp чтобы не тащить VehicleConstraint.h в заголовки.
// VehicleConstraint.h содержит JPH_IMPLEMENT_RTTI_VIRTUAL — крашит DLL при LoadLibraryA.

#include "PhysicsState.h"
#include "CarState.h"

namespace RKeng::CarConstraint
{
    void Register  (CarState& car, PhysicsState& ph);
    void Unregister(CarState& car, PhysicsState& ph);
}
