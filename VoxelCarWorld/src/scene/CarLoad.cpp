// CarLoad.cpp — создание физики RAM 2500 Power Wagon.
//
// ПРАВИЛА АРХИТЕКТУРЫ:
//   • Кузов SpawnDynamicBox — только через EngineAPI (реализация в RKengCore.dll)
//   • VehicleConstraint, WheelSettingsWV — напрямую (не используют Factory)
//   • BoxShapeSettings — НЕ вызываем! (требует Factory)
//   • JoltAssert.cpp — НЕ включаем (Jolt v5 сам определяет JPH::AssertFailed)

#include "scene/CarLoad.h"

#ifdef RK_JOLT_ENABLED
#  include <Jolt/Jolt.h>
#  include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#  include <Jolt/Physics/Vehicle/VehicleCollisionTester.h>
#  include <Jolt/Physics/Vehicle/WheeledVehicleController.h>
#  include <Jolt/Physics/Body/BodyLockInterface.h>
#  include <Jolt/Physics/Body/Body.h>
#endif

#include <cstdint>
#include <cstring>
#include <climits>

namespace RKeng::CarLoad
{
    // ── Инициализация воксельного кузова ──────────────────────────────────
    static void InitVoxels(CarState& car)
    {
        for (int x = 0; x < CAR_VX_W; x++)
        for (int y = 0; y < CAR_VX_H; y++)
        for (int z = 0; z < CAR_VX_L; z++)
        {
            auto& v = car.voxels[x][y][z];
            v.alive  = true;
            v.health = 1.0f;

            // Расцветка по зонам
            if (y == CAR_VX_H - 1)
            {
                // Крыша — тёмно-синяя / чёрная
                v.color = { 0.06f, 0.09f, 0.18f };
            }
            else if (y >= CAR_VX_H - 3 && z > 2 && z < CAR_VX_L - 3)
            {
                // Окна — серо-голубые (стекло)
                v.color = { 0.30f, 0.38f, 0.52f };
            }
            else if (z < 3)
            {
                // Капот / решётка — чёрный
                v.color = { 0.12f, 0.12f, 0.12f };
            }
            else if (z >= CAR_VX_L - 3)
            {
                // Задний борт — тёмный
                v.color = { 0.10f, 0.14f, 0.22f };
            }
            else if (y == 0)
            {
                // Порог / колёсные арки — тёмно-серый
                v.color = { 0.18f, 0.18f, 0.20f };
            }
            else
            {
                // Основной кузов — RAM тёмно-синий
                float brightness = 0.85f + (float)x / (float)(CAR_VX_W - 1) * 0.15f;
                v.color = { 0.12f * brightness, 0.22f * brightness, 0.45f * brightness };
            }
        }
        car.meshDirty = true;
    }

    // ── Создать кривую фрикции ────────────────────────────────────────────
#ifdef RK_JOLT_ENABLED
    static JPH::LinearCurve MakeFrictionCurve(float peak)
    {
        JPH::LinearCurve c;
        c.AddPoint(0.00f, 0.0f);
        c.AddPoint(0.06f, peak);
        c.AddPoint(0.18f, peak * 0.88f);
        c.AddPoint(0.50f, peak * 0.78f);
        c.AddPoint(1.00f, peak * 0.68f);
        return c;
    }

    static JPH::LinearCurve MakeLateralCurve(float peak)
    {
        JPH::LinearCurve c;
        c.AddPoint(0.00f, 0.0f);
        c.AddPoint(0.04f, peak);
        c.AddPoint(0.15f, peak * 0.82f);
        c.AddPoint(0.50f, peak * 0.65f);
        c.AddPoint(1.00f, peak * 0.55f);
        return c;
    }
#endif

    // ── Run ───────────────────────────────────────────────────────────────
    void Run(CarState& car, PhysicsState& ph, Vec3 spawnPos,
             const EngineAPI* api)
    {
        InitVoxels(car);

        car.damage.currentHP = car.damage.totalHP;
        car.damage.destroyed = false;
        car.position         = spawnPos;
        car.orientation      = Quat(1.0f, 0.0f, 0.0f, 0.0f);
        car.meshDirty        = true;

#ifdef RK_JOLT_ENABLED
        if (!ph.initialized)
        {
            if (api && api->LogWarn) api->LogWarn("CarLoad: physics not initialized");
            return;
        }

        // ── 1. Кузов через EngineAPI ──────────────────────────────────────
        if (!api || !api->SpawnDynamicBox)
        {
            if (api && api->LogError) api->LogError("CarLoad: EngineAPI::SpawnDynamicBox not bound");
            return;
        }

        const auto& p = car.params;

        RK_DynamicBox bodyDesc{};
        bodyDesc.cx             = spawnPos.x;
        bodyDesc.cy             = spawnPos.y;
        bodyDesc.cz             = spawnPos.z;
        bodyDesc.hx             = p.halfW;
        bodyDesc.hy             = p.halfH;
        bodyDesc.hz             = p.halfL;
        bodyDesc.mass           = p.mass;
        bodyDesc.linearDamping  = p.linearDamping;
        bodyDesc.angularDamping = p.angularDamping;
        bodyDesc.friction       = p.friction;

        uint32_t rawID = api->SpawnDynamicBox(ph, bodyDesc);
        if (rawID == UINT32_MAX)
        {
            if (api->LogError) api->LogError("CarLoad: SpawnDynamicBox returned UINT32_MAX");
            return;
        }
        car.bodyID = JPH::BodyID(rawID);

        // ── 2. VehicleConstraintSettings ─────────────────────────────────
        // VehicleConstraint не трогает Factory — безопасно в DLL-плагине.
        JPH::VehicleConstraintSettings vcs;
        vcs.mUp      = JPH::Vec3::sAxisY();
        vcs.mForward = JPH::Vec3::sAxisZ();

        // ── 3. Четыре колеса ──────────────────────────────────────────────
        // Расположение по реальному Power Wagon:
        //   колёсная база: 3.57 м → wZF = +1.785, wZR = -1.785
        //   ширина колеи:  1.60 м → wX  = ±0.80
        //   колесо ниже кузова на suspMaxLen + wheelRadius
        const float wX  = p.halfW + p.wheelWidth * 0.3f + 0.04f;
        const float wY  = -(p.halfH + p.suspMaxLen * 0.7f);
        const float wZF =  1.785f;
        const float wZR = -1.785f;

        JPH::WheelSettingsWV wTpl;
        wTpl.mRadius              = p.wheelRadius;
        wTpl.mWidth               = p.wheelWidth;
        wTpl.mSuspensionMinLength = p.suspMinLen;
        wTpl.mSuspensionMaxLength = p.suspMaxLen;
        wTpl.mSuspensionSpring.mFrequency = p.suspFreq;
        wTpl.mSuspensionSpring.mDamping   = p.suspDamping;
        wTpl.mSuspensionDirection = JPH::Vec3(0, -1,  0);
        wTpl.mSteeringAxis        = JPH::Vec3(0,  1,  0);
        wTpl.mWheelUp             = JPH::Vec3(0,  1,  0);
        wTpl.mWheelForward        = JPH::Vec3(0,  0,  1);

        auto fFric  = MakeFrictionCurve(p.frontFriction);
        auto rFric  = MakeFrictionCurve(p.rearFriction);
        auto fLat   = MakeLateralCurve(p.frontFriction);
        auto rLat   = MakeLateralCurve(p.rearFriction);

        auto MakeWheel = [&](float wx, float wz, float maxSteer,
                             const JPH::LinearCurve& lon,
                             const JPH::LinearCurve& lat) -> JPH::WheelSettingsWV*
        {
            auto* w = new JPH::WheelSettingsWV(wTpl);
            w->SetEmbedded();
            w->mPosition             = JPH::Vec3(wx, wY, wz);
            w->mMaxSteerAngle        = JPH::DegreesToRadians(maxSteer);
            w->mLongitudinalFriction = lon;
            w->mLateralFriction      = lat;
            return w;
        };

        // Wheel indices: 0=FL, 1=FR, 2=RL, 3=RR
        const float steer = p.maxSteerDeg;
        vcs.mWheels =
        {
            MakeWheel(-wX, wZF, steer, fFric, fLat),  // 0: FL
            MakeWheel( wX, wZF, steer, fFric, fLat),  // 1: FR
            MakeWheel(-wX, wZR, 0.0f,  rFric, rLat),  // 2: RL
            MakeWheel( wX, wZR, 0.0f,  rFric, rLat),  // 3: RR
        };

        // ── 4. WheeledVehicleController (8-ступ. AT) ─────────────────────
        auto* ctrl = new JPH::WheeledVehicleControllerSettings;
        ctrl->SetEmbedded();

        ctrl->mEngine.mMaxTorque = p.maxTorque;
        ctrl->mEngine.mMaxRPM    = p.maxRPM;
        ctrl->mEngine.mInertia   = p.engineInertia;

        ctrl->mTransmission.mMode = JPH::ETransmissionMode::Auto;
        // 8 передач вперёд + задний ход
        ctrl->mTransmission.mGearRatios = {
            p.gearRatios[0], p.gearRatios[1], p.gearRatios[2], p.gearRatios[3],
            p.gearRatios[4], p.gearRatios[5], p.gearRatios[6], p.gearRatios[7]
        };
        ctrl->mTransmission.mReverseGearRatios = { p.revGearRatio };
        ctrl->mTransmission.mSwitchTime        = p.switchTime;

        // 4WD: два дифференциала — передний и задний мост
        {
            JPH::VehicleDifferentialSettings diff;
            diff.mLeftWheel       = 0; diff.mRightWheel = 1;
            diff.mLimitedSlipRatio = p.diffSlipRatio;
            ctrl->mDifferentials.push_back(diff);

            diff.mLeftWheel       = 2; diff.mRightWheel = 3;
            ctrl->mDifferentials.push_back(diff);
        }

        // Антикрен
        {
            JPH::VehicleAntiRollBar arF;
            arF.mLeftWheel = 0; arF.mRightWheel = 1;
            arF.mStiffness = p.antiRollFront;
            vcs.mAntiRollBars.push_back(arF);

            JPH::VehicleAntiRollBar arR;
            arR.mLeftWheel = 2; arR.mRightWheel = 3;
            arR.mStiffness = p.antiRollRear;
            vcs.mAntiRollBars.push_back(arR);
        }

        vcs.mController = ctrl;

        // ── 5. VehicleConstraint ──────────────────────────────────────────
        {
            JPH::BodyLockWrite lock(ph.physicsSystem->GetBodyLockInterface(),
                                    car.bodyID);
            JPH::Body& body = lock.GetBody();
            auto* vc = new JPH::VehicleConstraint(body, vcs);
            vc->SetEmbedded();
            car.vehicleConstraint = vc;
        }

        // RayCast тестер — проверяет контакт колеса с полом
        auto* tester = new JPH::VehicleCollisionTesterRay(PhysLayers::STATIC);
        tester->SetEmbedded();
        car.vehicleConstraint->SetVehicleCollisionTester(tester);

        car.initialized = true;
        if (api->LogInfo) api->LogInfo("CarLoad: RAM 2500 Power Wagon body + VehicleConstraint created.");

#else
        (void)api;
        car.initialized = true;
        // no LogInfo available without api in no-physics path
        (void)ph;
#endif
    }

    // ── Destroy ───────────────────────────────────────────────────────────
    // ВАЖНО: вызывать ПОСЛЕ CarConstraint::Unregister (который делает Remove*).
    // Здесь только Release() указателя и уничтожение тела.
    void Destroy(CarState& car, PhysicsState& ph)
    {
#ifdef RK_JOLT_ENABLED
        if (!car.initialized || !ph.initialized || !ph.physicsSystem) return;

        if (car.vehicleConstraint)
        {
            // НЕ вызываем RemoveStepListener/RemoveConstraint здесь —
            // это делает CarConstraint::Unregister перед вызовом Destroy.
            // Двойной Remove → JPH_ASSERT.
            car.vehicleConstraint->Release();  // освобождаем Jolt ref-count
            car.vehicleConstraint = nullptr;
        }
        if (!car.bodyID.IsInvalid())
        {
            ph.bodyInterface->RemoveBody(car.bodyID);
            ph.bodyInterface->DestroyBody(car.bodyID);
        }
#else
        (void)ph;
#endif
        car.initialized = false;
    }

}  // namespace RKeng::CarLoad
