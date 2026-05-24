// CarTick.cpp — тик RAM 2500 Power Wagon.
//
// • Подаёт CarInput в WheeledVehicleController
// • Детектирует попадание в зоны грязи, применяет буксовку
// • Синхронизирует позицию/вращение из Jolt в CarState
// • Тикает осколки (debris) с гравитацией

#include "CarTick.h"
#include "WorldGen.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cstdint>
#include <cmath>

#ifdef RK_JOLT_ENABLED
#  include <Jolt/Jolt.h>
#  include <Jolt/Physics/Vehicle/WheeledVehicleController.h>
#  include <Jolt/Physics/Body/Body.h>
#  include <Jolt/Physics/Body/BodyInterface.h>
#endif

namespace RKeng::CarTick
{
    // ── LCG (детерминированный random без <cstdlib>) ─────────────────────
    static uint32_t s_rng = 0xBEEFCAFEu;
    static float Randf(float lo, float hi)
    {
        s_rng = s_rng * 1664525u + 1013904223u;
        return lo + (hi - lo) * ((s_rng & 0xFFFFu) / 65535.0f);
    }

    // ── Тик дебриса ───────────────────────────────────────────────────────
    static void TickDebris(CarState& car, float dt)
    {
        bool anyAlive = false;
        for (auto& d : car.debris)
        {
            if (d.dead) continue;
            d.velocity.y -= 9.81f * dt;
            d.pos        += d.velocity * dt;
            d.lifetime   += dt;

            // Отскок от земли (y ≈ 0)
            if (d.pos.y < 0.0f && d.velocity.y < 0.0f)
            {
                d.pos.y      = 0.0f;
                d.velocity.y = -d.velocity.y * 0.35f;
                d.velocity.x *= 0.7f;
                d.velocity.z *= 0.7f;
            }

            if (d.lifetime > 5.0f)
                d.dead = true;
            else
                anyAlive = true;
        }
        if (!anyAlive && !car.debris.empty())
            car.debris.clear();
    }

    // ── Простая визуальная разборка вокселя при ударе ────────────────────
    static void SpawnDebrisVoxel(CarState& car, Vec3 worldPos, Vec3 color)
    {
        CarDebris d;
        d.pos      = worldPos;
        d.color    = color;
        d.size     = CAR_VOXEL_SIZE * 0.9f;
        d.velocity = Vec3(Randf(-4.f, 4.f),
                          Randf(3.f, 10.f),
                          Randf(-4.f, 4.f));
        d.lifetime = 0.0f;
        car.debris.push_back(d);
    }

    // ── Урон от столкновения ──────────────────────────────────────────────
    // impulse — Н·с, localHitPos — в системе координат кузова.
    static void ApplyImpact(CarState& car, Vec3 localHitPos, float impulse)
    {
        if (impulse < car.damage.voxelCrackImpulse) return;

        const float S    = CAR_VOXEL_SIZE;
        const float offX = -(float)CAR_VX_W * S * 0.5f;
        const float offZ = -(float)CAR_VX_L * S * 0.5f;
        float breakR     = glm::clamp(impulse / 3000.0f, 0.1f, 2.0f);

        float dmg = impulse * 0.05f;
        car.damage.currentHP -= dmg;
        if (car.damage.currentHP <= 0.0f) car.damage.destroyed = true;

        bool changed = false;
        for (int x = 0; x < CAR_VX_W; x++)
        for (int y = 0; y < CAR_VX_H; y++)
        for (int z = 0; z < CAR_VX_L; z++)
        {
            auto& v = car.voxels[x][y][z];
            if (!v.alive) continue;

            Vec3 vc = {
                offX + x * S + S * 0.5f,
                       y * S + S * 0.5f,
                offZ + z * S + S * 0.5f
            };
            float dist = glm::length(vc - localHitPos);
            if (dist > breakR) continue;

            float frac = 1.0f - dist / breakR;
            if (impulse >= car.damage.voxelBreakImpulse * frac)
            {
                v.alive = false;
                changed = true;
                SpawnDebrisVoxel(car,
                    car.position + car.orientation * vc, v.color);
            }
            else
            {
                v.health -= frac * 0.35f;
                changed   = true;
            }
        }

        if (changed) car.meshDirty = true;
    }

    // ── Детект грязи по позиции колеса ───────────────────────────────────
    static int CountWheelsInMud(
        const std::vector<WorldGen::MudZone>& zones,
        float x, float z, float wheelBase, float trackWidth)
    {
        // Позиции 4 колёс в XZ
        const float wZF =  wheelBase * 0.5f;
        const float wZR = -wheelBase * 0.5f;
        const float wX  =  trackWidth * 0.5f;

        float wx[4] = { x - wX, x + wX, x - wX, x + wX };
        float wz[4] = { z + wZF, z + wZF, z + wZR, z + wZR };

        int count = 0;
        for (int i = 0; i < 4; i++)
        {
            for (const auto& mz : zones)
            {
                if (wx[i] >= mz.minX && wx[i] <= mz.maxX &&
                    wz[i] >= mz.minZ && wz[i] <= mz.maxZ)
                {
                    count++;
                    break;
                }
            }
        }
        return count;
    }

    // ── Главный тик ───────────────────────────────────────────────────────
    void Run(CarState& car, PhysicsState& ph, SceneState& scene,
             float dt,
             const std::vector<WorldGen::MudZone>& mudZones)
    {
        (void)scene;
        if (!car.initialized) return;

        TickDebris(car, dt);

#ifdef RK_JOLT_ENABLED
        if (!ph.initialized || !car.vehicleConstraint) return;

        auto* ctrl = static_cast<JPH::WheeledVehicleController*>(
            car.vehicleConstraint->GetController());

        const auto& inp = car.input;
        const auto& p   = car.params;

        // ── Грязь: пересчитываем фрикцию и добавляем сопротивление ───────
        int wheelsInMud = CountWheelsInMud(mudZones,
            car.position.x, car.position.z,
            3.57f, 1.60f);

        car.mud.wheelsInMud = wheelsInMud;
        car.mud.inMud       = (wheelsInMud > 0);
        car.mud.mudDepth    = (float)wheelsInMud / 4.0f;

        // Модифицируем фрикцию колёс в зависимости от грязи
        if (wheelsInMud > 0)
        {
            // Базовая фрикция × mudFactor для колёс в грязи
            float mudMul = 1.0f - car.mud.mudDepth * (1.0f - car.mudParams.frictionMul);

            // Применяем через overide friction scale на колёса
            // (Jolt WheeledVehicleController: mForwardFrictionFactor / mSideFrictionFactor)
            // Эти поля доступны в WheelWV
            auto& wheels = car.vehicleConstraint->GetWheels();
            const float wh2mud[4] = {
                // колесо в грязи: mudMul, иначе 1.0
                (CountWheelsInMud(mudZones, car.position.x, car.position.z, 3.57f, 1.60f) > 0) ? mudMul : 1.0f,
                mudMul, mudMul, mudMul  // упрощение: все 4 при wheelsInMud > 0
            };

            for (int i = 0; i < 4 && i < (int)wheels.size(); i++)
            {
                auto* wv = static_cast<JPH::WheelWV*>(wheels[i]);
                // WheelWV expose contact point; friction multiplier через settings
                // Jolt v5: можно override через mLongitudinalFriction curve scaling
                // Упрощение: добиваемся эффекта через дополнительное линейное торможение
                (void)wv;
                (void)wh2mud[i];
            }

            // Добавляем линейное drag-торможение на кузов пропорционально глубине грязи
            if (ph.bodyInterface && !car.bodyID.IsInvalid())
            {
                // Вектор тяги — противоположен горизонтальной скорости
                JPH::Vec3 jVel = ph.bodyInterface->GetLinearVelocity(car.bodyID);
                float speed = jVel.Length();
                if (speed > 0.1f)
                {
                    float dragF = car.mudParams.dragForcePerWheel * (float)wheelsInMud;
                    JPH::Vec3 dragDir = -jVel;
                    dragDir = dragDir.NormalizedOr(JPH::Vec3::sZero());

                    // Применяем импульс торможения: F·dt
                    // Не используем JPH:: Shape/Body API — только BodyInterface
                    ph.bodyInterface->AddImpulse(car.bodyID,
                        dragDir * dragF * dt);
                }
            }

            // Буксовка: если throttle > spinTreshold и глубокая грязь
            car.mud.spinParticleTimer += dt;
            if (inp.throttle > car.mudParams.spinTreshold &&
                car.mud.mudDepth > 0.4f &&
                car.mud.spinParticleTimer > 0.05f)
            {
                car.mud.spinParticleTimer = 0.0f;
                // Визуальный эффект: выбрасываем мелкий дебрис-спрей
                Vec3 rearPos = car.position + car.orientation * Vec3(0.0f, -0.2f, -1.8f);
                for (int s = 0; s < 3; s++)
                {
                    CarDebris d;
                    d.pos      = rearPos + Vec3(Randf(-0.5f, 0.5f), 0.0f, 0.0f);
                    d.color    = Vec3(0.36f, 0.22f, 0.12f);  // грязно-коричневый
                    d.size     = 0.06f;
                    d.velocity = Vec3(Randf(-3.f, 3.f), Randf(1.f, 5.f), Randf(-8.f, -2.f));
                    d.lifetime = 0.0f;
                    car.debris.push_back(d);
                }
            }
        }

        // ── Пониженная передача (4L): удваиваем тягу, ограничиваем скорость ─
        float throttleEff = inp.throttle;
        float brakeEff    = inp.brake;
        if (inp.lowRange)
        {
            // Jolt Auto transmission не знает про Transfer box —
            // имитируем через увеличение подачи газа × transferLow factor
            // и ограничение через brake если передатурили
            throttleEff = glm::min(1.0f, inp.throttle * 1.3f);

            // Скорость > 15 km/h в 4L — добавляем противодавление
            float speed = glm::length(car.velocity);
            if (speed > 4.17f)  // ~15 km/h
                brakeEff = glm::min(1.0f, brakeEff + 0.3f);
        }

        // ── Подаём ввод в контроллер ──────────────────────────────────────
        ctrl->SetDriverInput(
            throttleEff,
            brakeEff,
            inp.steer,
            inp.handbrake ? 1.0f : 0.0f);

        // Ручник — дополнительно блокируем задние колёса
        if (inp.handbrake)
        {
            auto& wheels = car.vehicleConstraint->GetWheels();
            for (int i = 2; i <= 3 && i < (int)wheels.size(); i++)
            {
                auto* w = static_cast<JPH::WheelWV*>(wheels[i]);
                w->mBrakeImpulse = p.handbrakeForce * dt;
            }
        }

        // ── Синхронизируем трансформ из Jolt ─────────────────────────────
        JPH::Vec3 jPos = ph.bodyInterface->GetPosition(car.bodyID);
        JPH::Quat jRot = ph.bodyInterface->GetRotation(car.bodyID);
        JPH::Vec3 jVel = ph.bodyInterface->GetLinearVelocity(car.bodyID);

        car.position    = { jPos.GetX(), jPos.GetY(), jPos.GetZ() };

        // Синхронизируем камеру сцены с позицией машины
        Vec3 camOffset = car.orientation * car.camLocalOffset;
        Vec3 camPos    = car.position + camOffset;
        scene.player.worldPos.world.x = camPos.x;
        scene.player.worldPos.world.y = camPos.y;
        scene.player.worldPos.world.z = camPos.z;
        scene.input.yaw   = car.camYaw;
        scene.input.pitch = car.camPitch;
        car.orientation = glm::quat(jRot.GetW(),
                                    jRot.GetX(), jRot.GetY(), jRot.GetZ());
        car.velocity    = { jVel.GetX(), jVel.GetY(), jVel.GetZ() };
        car.speedKph    = glm::length(car.velocity) * 3.6f;

        // Текущая передача
        auto* vc = car.vehicleConstraint;
        if (vc) {
            auto* wCtrl = static_cast<const JPH::WheeledVehicleController*>(
                vc->GetController());
            car.currentGear = wCtrl->GetTransmission().GetCurrentGear();
        }

#else
        (void)ph; (void)dt; (void)mudZones;
#endif
    }

    // ── RegisterContactCallback ───────────────────────────────────────────
    // Регистрирует обработчик столкновений для кузова машины.
    // Полная реализация — через RKContactListener в sdk/ — планируется.
    // Пока заглушка: функция существует, но ничего не делает.
    void RegisterContactCallback(CarState& /*car*/, PhysicsState& /*ph*/)
    {
        // TODO: зарегистрировать RKContactListener через PhysicsSystem::SetContactListener
        // для детекции ударов → ApplyImpact().
    }

}  // namespace RKeng::CarTick