// CarConstraint.cpp — Register/Unregister VehicleConstraint в PhysicsSystem.
//
// ПОЧЕМУ ОТДЕЛЬНЫЙ ФАЙЛ:
//   VehicleConstraint.h содержит JPH_IMPLEMENT_RTTI_VIRTUAL — макрос,
//   создающий глобальные C++ объекты с нетривиальными конструкторами.
//   Если этот include попадает в .cpp где есть ЛЮБОЙ глобальный объект
//   с конструктором, Windows инициализирует их при LoadLibraryA ДО DllMain,
//   обращается к JPH::Factory::sInstance (ещё nullptr) → 0x80000003.
//
//   В этом файле нет глобальных переменных → RTTI-объекты инициализируются
//   безопасно при первом вызове функции (после InitJoltFromEngine).

#include "scene/CarConstraint.h"

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
        // Порядок строго по правилу 6: RemoveStepListener → RemoveConstraint → Release
        ph.physicsSystem->RemoveStepListener(car.vehicleConstraint);
        ph.physicsSystem->RemoveConstraint(car.vehicleConstraint);
        car.vehicleConstraint->Release();
        car.vehicleConstraint = nullptr;
#endif
    }
}
