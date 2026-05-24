// CarTick.cpp — тик машины: ввод → VehicleController → синхронизация трансформа → камера.

#include "CarTick.h"

#ifdef RK_JOLT_ENABLED
#  include <Jolt/Jolt.h>
#  include <Jolt/Physics/Vehicle/WheeledVehicleController.h>
#  include <Jolt/Physics/Body/BodyInterface.h>
#endif

#include <glm/gtc/quaternion.hpp>
#include <cmath>

namespace RKeng::CarTick
{
    void Run(CarState& car, PhysicsState& ph, SceneState& scene, float dt)
    {
        if (!car.initialized) return;
        (void)dt;

#ifdef RK_JOLT_ENABLED
        if (!ph.initialized || !car.vehicleConstraint || !ph.bodyInterface) return;

        auto* ctrl = static_cast<JPH::WheeledVehicleController*>(
            car.vehicleConstraint->GetController());

        ctrl->SetDriverInput(
            car.input.throttle,
            car.input.brake,
            car.input.steer,
            car.input.handbrake ? 1.0f : 0.0f);

        // Синхронизируем трансформ из Jolt
        JPH::RVec3 jPos = ph.bodyInterface->GetPosition(car.bodyID);
        JPH::Quat  jRot = ph.bodyInterface->GetRotation(car.bodyID);
        JPH::Vec3  jVel = ph.bodyInterface->GetLinearVelocity(car.bodyID);

        car.position    = Vec3((float)jPos.GetX(), (float)jPos.GetY(), (float)jPos.GetZ());
        car.orientation = glm::quat(jRot.GetW(), jRot.GetX(), jRot.GetY(), jRot.GetZ());
        car.velocity    = Vec3(jVel.GetX(), jVel.GetY(), jVel.GetZ());
        car.speedKph    = glm::length(car.velocity) * 3.6f;
#else
        (void)ph;
#endif

        // Камера — третье лицо
        // camYaw/camPitch уже записаны CarInputPoll.
        // Вычисляем позицию камеры в мировом пространстве.
        float yawR   = car.camYaw   * DEG2RAD;
        float pitchR = car.camPitch * DEG2RAD;

        // Орбитальная камера вокруг центра машины
        float cosP = std::cos(pitchR);
        Vec3 camOffset = Vec3(
            -std::sin(yawR) * cosP,
             std::sin(pitchR),
            -std::cos(yawR) * cosP
        ) * car.camDist;

        // Добавляем небольшой Y-офсет (смотрим чуть выше кузова)
        Vec3 camTarget = car.position + Vec3(0.0f, car.params.halfH * 0.5f, 0.0f);
        Vec3 camPos    = camTarget + camOffset;

        scene.player.worldPos.world.x = camPos.x;
        scene.player.worldPos.world.y = camPos.y;
        scene.player.worldPos.world.z = camPos.z;

        // Направление взгляда → yaw/pitch для рендера
        Vec3 dir = glm::normalize(camTarget - camPos);
        scene.input.yaw   = std::atan2(-dir.x, -dir.z) * RAD2DEG;
        scene.input.pitch = std::asin(dir.y)            * RAD2DEG;
    }
}
