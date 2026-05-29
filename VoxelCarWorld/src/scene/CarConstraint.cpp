// CarConstraint.cpp — Register/Unregister теперь не нужны:
// SpawnVehicle в движке уже вызывает AddConstraint+AddStepListener,
// DestroyVehicle — RemoveStepListener+RemoveConstraint.
// Файл оставлен для совместимости; функции — no-op.

#include "CarConstraint.h"

namespace RKeng::CarConstraint
{
    void Register(CarState& /*car*/, PhysicsState& /*ph*/)
    {
        // Сделано внутри SpawnVehicle в движке.
    }

    void Unregister(CarState& /*car*/, PhysicsState& /*ph*/)
    {
        // Сделано внутри DestroyVehicle в движке.
    }
}
