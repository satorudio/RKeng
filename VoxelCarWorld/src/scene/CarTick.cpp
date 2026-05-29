// CarTick.cpp — тик машины через EngineAPI, камера от первого лица.

#include "CarTick.h"

#include <glm/gtc/quaternion.hpp>
#include <cmath>
#include <GLFW/glfw3.h>
#include <string>

namespace RKeng::CarTick
{
    void Run(CarState& car, PhysicsState& ph, SceneState& scene,
             float dt, const EngineAPI& api)
    {
        if (!car.initialized) return;
        (void)dt;

        // Инпут водителя → движок
        if (api.SetVehicleInput) {
            RK_VehicleInput inp{};
            inp.throttle  = car.input.throttle;
            inp.brake     = car.input.brake;
            inp.steer     = car.input.steer;
            inp.handbrake = car.input.handbrake ? 1.f : 0.f;
            api.SetVehicleInput(ph, car.vehicleHandle, inp);
        }

        // Трансформ ← движок
        if (api.GetVehicleTransform) {
            float px, py, pz, qx, qy, qz, qw, vx, vy, vz;
            if (api.GetVehicleTransform(ph, car.vehicleHandle,
                                        px, py, pz,
                                        qx, qy, qz, qw,
                                        vx, vy, vz))
            {
                car.position    = Vec3(px, py, pz);
                car.orientation = glm::quat(qw, qx, qy, qz);
                car.velocity    = Vec3(vx, vy, vz);
                car.speedKph    = glm::length(car.velocity) * 3.6f;
            }
        }

        // Камера — первое лицо: сидим в кабине водителя
        // Смещение в локальных координатах машины: вперёд и вверх
        const Vec3 cockpitLocal(0.0f,
                                 car.params.halfH * 1.6f,   // высота — на уровне глаз
                                 car.params.halfL * 0.5f);  // чуть вперёд от центра

        // Переводим смещение в мировые координаты через кватернион машины
        Vec3 camPos = car.position + car.orientation * cockpitLocal;

        scene.player.worldPos.world.x = camPos.x;
        scene.player.worldPos.world.y = camPos.y;
        scene.player.worldPos.world.z = camPos.z;
        // thirdPersonCamera=true — движок не добавляет currentHeight к Y, позиция финальная

        // Направление взгляда: базовый yaw от машины + yaw мыши, pitch мыши
        // Извлекаем yaw машины из кватерниона
        const float carYaw = std::atan2(
            2.0f * (car.orientation.w * car.orientation.y + car.orientation.z * car.orientation.x),
            1.0f - 2.0f * (car.orientation.y * car.orientation.y + car.orientation.z * car.orientation.z)
        ) * RAD2DEG;

        scene.input.yaw   = carYaw + car.camYaw;
        scene.input.pitch = car.camPitch;

        // Скорость в заголовке окна для отладки
        GLFWwindow* win = scene.windowHandle;
        if (win) {
            std::string title = "RKeng  |  " + std::to_string((int)car.speedKph) + " km/h"
                              + "  throttle=" + std::to_string(car.input.throttle).substr(0,4)
                              + "  steer=" + std::to_string(car.input.steer).substr(0,5);
            glfwSetWindowTitle(win, title.c_str());
        }
    }
}
