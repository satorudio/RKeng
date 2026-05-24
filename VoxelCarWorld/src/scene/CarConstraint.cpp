// CarConstraint.cpp — ТОЛЬКО Register/Unregister. Никаких глобальных переменных.

#include "CarConstraint.h"

#ifdef RK_JOLT_ENABLED
#  include <Jolt/Jolt.h>
#  include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#endif

namespace RKeng::CarConstraint
{
    void Register(CarState& car, PhysicsState& ph)
    {
#ifdef RK_JOLT_ENABLED
        if (!car.vehicleConstraint || !ph.physicsSystem) return;
        ph.physicsSystem->AddConstraint(car.vehicleConstraint);
        ph.physicsSystem->AddStepListener(car.vehicleConstraint);
#endif
    }

    void Unregister(CarState& car, PhysicsState& ph)
    {
#ifdef RK_JOLT_ENABLED
        if (!car.vehicleConstraint || !ph.physicsSystem) return;
        // Порядок: RemoveStepListener → RemoveConstraint → Release
        ph.physicsSystem->RemoveStepListener(car.vehicleConstraint);
        ph.physicsSystem->RemoveConstraint(car.vehicleConstraint);
        car.vehicleConstraint->Release();
        car.vehicleConstraint = nullptr;
#endif
    }
}
