#include "CarTick.h"
#include "CarMesh.h"
#include "../utils/Logger.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cstdint>

#ifdef RK_JOLT_ENABLED
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Vehicle/WheeledVehicleController.h>
#include <Jolt/Physics/Body/Body.h>
#endif

namespace RKeng::CarTick
{
    // ----------------------------------------------------------------
    //  Утилиты — потокобезопасный LCG вместо rand()
    // ----------------------------------------------------------------
    static uint32_t s_rngState = 0xDEADBEEFu;

    static float Randf(float lo, float hi)
    {
        s_rngState = s_rngState * 1664525u + 1013904223u;
        float t = (s_rngState & 0xFFFFu) / 65535.0f;
        return lo + (hi - lo) * t;
    }

    // ----------------------------------------------------------------
    //  Обновление дебриса (летящие куски)
    // ----------------------------------------------------------------
    static void TickDebris(CarState& car, float dt)
    {
        bool anyAlive = false;
        for (auto& d : car.debris)
        {
            if (d.dead) continue;
            d.velocity.y  -= 9.81f * dt;       // гравитация
            d.pos         += d.velocity * dt;
            d.lifetime    += dt;
            if (d.lifetime > 4.0f || d.pos.y < -20.0f)
                d.dead = true;
            else
                anyAlive = true;
        }
        // Убираем мёртвые
        if (!anyAlive && !car.debris.empty())
            car.debris.clear();
    }

    // ----------------------------------------------------------------
    //  Выбить вокселы при импульсе
    // ----------------------------------------------------------------
    static void ApplyImpactDamage(CarState& car, Vec3 hitLocalPos,
                                  float impulse, Vec3 hitDir)
    {
        const float S = CAR_VOXEL_SIZE;
        const float offX = -CAR_VOXELS_W * S * 0.5f;
        const float offZ = -CAR_VOXELS_L * S * 0.5f;

        float dmg = impulse / car.damage.totalHP;
        car.damage.currentHP -= dmg;
        if (car.damage.currentHP <= 0.0f) car.damage.destroyed = true;

        // Радиус разрушения пропорционален импульсу
        float breakRadius = glm::clamp(impulse / 2000.0f, 0.1f, 1.0f);
        bool meshChanged  = false;

        for (int x = 0; x < CAR_VOXELS_W; x++)
        for (int y = 0; y < CAR_VOXELS_H; y++)
        for (int z = 0; z < CAR_VOXELS_L; z++)
        {
            auto& v = car.voxels[x][y][z];
            if (!v.alive) continue;

            float cx = offX + x * S + S * 0.5f;
            float cy =        y * S + S * 0.5f;
            float cz = offZ + z * S + S * 0.5f;

            float dist = glm::length(Vec3(cx,cy,cz) - hitLocalPos);
            if (dist > breakRadius) continue;

            float impactFraction = 1.0f - dist / breakRadius;

            if (impulse >= car.damage.voxelBreakImpulse * impactFraction)
            {
                v.alive = false;
                meshChanged = true;

                // Создаём дебрис
                CarDebris d;
                d.pos      = car.position + Vec3(cx,cy,cz);
                d.color    = v.color;
                d.size     = S * 0.9f;
                // Импульс вылета
                d.velocity = hitDir * (8.0f + Randf(0,8.0f))
                           + Vec3(Randf(-3,3), Randf(2,10), Randf(-3,3));
                d.angularVel = Vec3(Randf(-5,5), Randf(-5,5), Randf(-5,5));
                car.debris.push_back(d);
            }
            else if (impulse >= car.damage.voxelCrackImpulse * impactFraction)
            {
                // Только трещина — снижаем здоровье вокселя
                v.health  -= impactFraction * 0.4f;
                meshChanged = true;
            }
        }

        if (meshChanged)
            CarMesh::Rebuild(car);
    }

    // ----------------------------------------------------------------
    //  Регистрация ContactListener — вызывается из CarLoad после
    //  создания PhysicsState и CarState (когда car.bodyID уже известен)
    // ----------------------------------------------------------------
    void RegisterContactCallback(CarState& car, PhysicsState& ph)
    {
#ifdef RK_JOLT_ENABLED
        if (!ph.contactListener) return;

        ph.contactListener->SetHitCallback(
            [&car](JPH::BodyID id1, JPH::BodyID id2,
                   JPH::Vec3 contactPoint, float impulse)
            {
                // Фильтруем: реагируем только на касания нашего тела
                if (id1 != car.bodyID && id2 != car.bodyID) return;

                // Переводим точку контакта в локальное пространство машины
                glm::vec3 worldPt = { contactPoint.GetX(),
                                      contactPoint.GetY(),
                                      contactPoint.GetZ() };
                glm::vec3 hitLocal = glm::inverse(car.orientation)
                                   * (worldPt - car.position);

                // Направление удара: от точки контакта к центру машины
                glm::vec3 hitDir = glm::normalize(car.position - worldPt);
                if (glm::length(hitDir) < 0.001f)
                    hitDir = car.orientation * Vec3(0, 0, 1);

                ApplyImpactDamage(car, hitLocal, impulse, hitDir);
            });
#else
        (void)car; (void)ph;
#endif
    }

    // ----------------------------------------------------------------
    //  Главный тик
    // ----------------------------------------------------------------
    void Run(CarState& car, PhysicsState& ph, SceneState& scene, float dt)
    {
        if (!car.initialized) return;

        // Дебрис обновляем всегда (не зависит от физики)
        TickDebris(car, dt);

#ifdef RK_JOLT_ENABLED
        if (!ph.initialized || car.vehicleConstraint == nullptr) return;

        auto* ctrl = static_cast<JPH::WheeledVehicleController*>(
            car.vehicleConstraint->GetController());

        // ---- Управление ------------------------------------------------
        const auto& inp = car.input;

        // Газ / тормоз
        ctrl->SetDriverInput(
            inp.throttle,   // forward (0..1)
            inp.brake,      // brake   (0..1)
            inp.steer,      // right = positive (-1..1)
            inp.handbrake ? 1.0f : 0.0f);

        // Ручник — дополнительно блокируем задние колёса
        if (inp.handbrake)
        {
            auto& wheels = car.vehicleConstraint->GetWheels();
            for (int i = 2; i <= 3; i++) // RL, RR
            {
                auto* w = static_cast<JPH::WheelWV*>(wheels[i]);
                w->mBrakeImpulse = car.params.handbrakeForce * dt;
            }
        }

        // ---- Считываем состояние тела ----------------------------------
        JPH::Vec3 jPos = ph.bodyInterface->GetPosition(car.bodyID);
        JPH::Quat jRot = ph.bodyInterface->GetRotation(car.bodyID);
        JPH::Vec3 jVel = ph.bodyInterface->GetLinearVelocity(car.bodyID);

        car.position    = { jPos.GetX(), jPos.GetY(), jPos.GetZ() };
        car.orientation = glm::quat(jRot.GetW(), jRot.GetX(), jRot.GetY(), jRot.GetZ());
        car.velocity    = { jVel.GetX(), jVel.GetY(), jVel.GetZ() };
        car.speedKph    = glm::length(car.velocity) * 3.6f;

        // Удары обрабатываются через RKContactListener::OnContactAdded,
        // который вызывает ApplyImpactDamage напрямую.
        // deltaV-хак удалён — он давал ложные срабатывания при торможении.

        // ---- Камера не управляется отсюда --------------------------------
        // Игрок ходит пешком — камера управляется в PlayerMove/InputPoll.
        // CarTick не трогает scene.input.yaw/pitch чтобы не конфликтовать.
        (void)scene;

#else
        (void)ph; (void)scene; (void)dt;
#endif
    }
}
