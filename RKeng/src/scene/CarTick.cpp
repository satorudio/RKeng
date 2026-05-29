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
        // ── Физика каждого осколка ────────────────────────────────────────
        for (auto& d : car.debris)
        {
            if (d.dead) continue;

            d.velocity.y -= 22.0f * dt;
            d.velocity   *= (1.0f - 0.55f * dt);
            d.pos        += d.velocity * dt;
            d.lifetime   += dt;

            // Отскок от пола
            if (d.pos.y < d.size * 0.5f && d.velocity.y < 0.0f)
            {
                d.pos.y      = d.size * 0.5f;
                d.velocity.y = -d.velocity.y * 0.32f;
                d.velocity.x *=  0.55f;
                d.velocity.z *=  0.55f;
                if (std::abs(d.velocity.y) < 1.2f)
                    d.velocity.y = 0.0f;
            }

            if (d.lifetime > 4.5f || d.pos.y < -30.0f)
                d.dead = true;
        }

        // ── Сепарация: не даём осколкам проваливаться друг в друга ───────
        // O(n²) — нормально при n < 50
        const int n = (int)car.debris.size();
        for (int i = 0; i < n; i++)
        {
            auto& a = car.debris[i];
            if (a.dead) continue;
            for (int j = i + 1; j < n; j++)
            {
                auto& b = car.debris[j];
                if (b.dead) continue;

                float minDist = (a.size + b.size) * 0.5f;
                Vec3  delta   = b.pos - a.pos;
                float dist    = glm::length(delta);
                if (dist < 0.0001f || dist >= minDist) continue;

                // Раздвигаем по оси столкновения
                Vec3  axis  = delta / dist;
                float push  = (minDist - dist) * 0.5f;
                a.pos -= axis * push;
                b.pos += axis * push;

                // Обмениваем компоненты скорости по оси (упругий удар, e=0.3)
                float va = glm::dot(a.velocity, axis);
                float vb = glm::dot(b.velocity, axis);
                if (va - vb > 0.0f) continue; // уже расходятся
                constexpr float e = 0.3f;
                float va2 = (va + vb + e * (vb - va)) * 0.5f;
                float vb2 = (va + vb + e * (va - vb)) * 0.5f;
                a.velocity += axis * (va2 - va);
                b.velocity += axis * (vb2 - vb);
            }
        }

        // Убираем мёртвые
        bool anyAlive = false;
        for (auto& d : car.debris)
            if (!d.dead) { anyAlive = true; break; }
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
                // Радиальный вылет от точки удара — каждый осколок летит
                // в свою сторону, пропорционально расстоянию от эпицентра
                Vec3 fromImpact = Vec3(cx,cy,cz) - hitLocalPos;
                float dist = glm::length(fromImpact);
                Vec3 radial = (dist > 0.01f)
                    ? glm::normalize(fromImpact)
                    : Vec3(Randf(-1,1), Randf(0.2f,1), Randf(-1,1));

                float speed = 14.0f + Randf(0, 18.0f) + impulse * 0.01f;

                d.velocity = radial  * speed
                           + hitDir  * Randf(3, 8)
                           + Vec3(Randf(-5, 5),
                                  Randf(4, 20),
                                  Randf(-5, 5));
                d.angularVel = Vec3(Randf(-18,18), Randf(-18,18), Randf(-18,18));
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
