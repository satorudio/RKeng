#pragma once
#include "types.h"
#include <cstdint>

namespace RKeng {

    // Хэндл на созданное транспортное средство.

    struct VehicleAPI {
        // Создать машину + зарегистрировать в PhysicsSystem (AddConstraint+AddStepListener).
        // Возвращает RK_INVALID_VEHICLE при ошибке.
        RK_VehicleHandle (*SpawnVehicle)(RK_WorldHandle world, const RK_VehicleDesc& desc) = nullptr;

        // Установить инпут водителя (вызывать каждый тик до шага физики).
        void (*SetVehicleInput)(RK_WorldHandle world, RK_VehicleHandle vh,
                                const RK_VehicleInput& inp) = nullptr;

        // Получить трансформ кузова (позиция + кватернион) и скорость.
        bool (*GetVehicleTransform)(RK_WorldHandle world, RK_VehicleHandle vh,
                                    float& px, float& py, float& pz,
                                    float& qx, float& qy, float& qz, float& qw,
                                    float& vx, float& vy, float& vz) = nullptr;

        // Удалить машину (RemoveStepListener + RemoveConstraint + DestroyBody + освободить слот).
        void (*DestroyVehicle)(RK_WorldHandle world, RK_VehicleHandle vh) = nullptr;
    };
}
