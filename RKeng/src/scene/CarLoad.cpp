#include "CarLoad.h"
#include "../utils/Logger.h"
#include "CarMesh.h"

#ifdef RK_JOLT_ENABLED
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#include <Jolt/Physics/Vehicle/VehicleCollisionTester.h>
#include <Jolt/Physics/Vehicle/WheeledVehicleController.h>
#endif

namespace RKeng::CarLoad
{
    void Run(CarState& car, PhysicsState& ph, Vec3 spawnPos)
    {
        // Инициализируем вокселы кузова
        for (int x = 0; x < CAR_VOXELS_W; x++)
        for (int y = 0; y < CAR_VOXELS_H; y++)
        for (int z = 0; z < CAR_VOXELS_L; z++)
        {
            auto& v = car.voxels[x][y][z];
            v.alive  = true;
            v.health = 1.0f;

            // Цвет: крыша темнее, низ светлее, капот другой цвет
            if (y == CAR_VOXELS_H - 1)
                v.color = { 0.6f, 0.05f, 0.05f };          // крыша — тёмно-красная
            else if (z < 2)
                v.color = { 0.2f, 0.2f, 0.25f };           // капот — тёмный
            else if (z >= CAR_VOXELS_L - 2)
                v.color = { 0.15f, 0.15f, 0.2f };          // корма — тёмная
            else
                v.color = { 0.9f + x*0.02f, 0.08f, 0.08f };// основной кузов
        }

        car.damage.currentHP = car.damage.totalHP;
        car.damage.destroyed = false;
        car.position         = spawnPos;
        car.orientation      = { 1,0,0,0 };
        car.meshDirty        = true;
        CarMesh::Rebuild(car);

#ifdef RK_JOLT_ENABLED
        if (!ph.initialized) { Logger::Warn("CarLoad: physics not initialized"); return; }

        auto& p = car.params;

        // ---- 1. Кузов — BoxShape ----------------------------------------
        JPH::BoxShapeSettings bodyShapeSettings(
            JPH::Vec3(p.halfExtentX, p.halfExtentY, p.halfExtentZ));
        bodyShapeSettings.SetEmbedded();
        auto bodyShapeResult = bodyShapeSettings.Create();
        if (bodyShapeResult.HasError()) {
            Logger::Error("CarLoad: BoxShape error");
            return;
        }

        JPH::BodyCreationSettings bcs(
            bodyShapeResult.Get(),
            JPH::RVec3(spawnPos.x, spawnPos.y, spawnPos.z),
            JPH::Quat::sIdentity(),
            JPH::EMotionType::Dynamic,
            PhysLayers::DYNAMIC);
        bcs.mOverrideMassProperties         = JPH::EOverrideMassProperties::CalculateInertia;
        bcs.mMassPropertiesOverride.mMass   = p.mass;
        bcs.mLinearDamping                  = 0.05f;
        bcs.mAngularDamping                 = 0.4f;
        bcs.mFriction                       = 0.3f;

        car.bodyID = ph.bodyInterface->CreateAndAddBody(bcs, JPH::EActivation::Activate);

        // ---- 2. VehicleConstraintSettings -----------------------------------
        JPH::VehicleConstraintSettings vcs;
        vcs.mUp      = JPH::Vec3::sAxisY();
        vcs.mForward = JPH::Vec3::sAxisZ();   // +Z = вперёд в локальном пространстве

        // ---- 3. Четыре колеса -------------------------------------------
        // Позиции колёс относительно центра кузова
        const float wX  =  p.halfExtentX + p.wheelWidth * 0.5f + 0.02f;
        const float wY  = -p.halfExtentY - p.suspensionMaxLen * 0.5f;
        const float wZF =  p.halfExtentZ * 0.65f;   // передняя ось
        const float wZR = -p.halfExtentZ * 0.65f;   // задняя ось

        // Подвеска — одинакова для всех колёс
        JPH::WheelSettingsWV wheelTemplate;
        wheelTemplate.mRadius               = p.wheelRadius;
        wheelTemplate.mWidth                = p.wheelWidth;
        wheelTemplate.mSuspensionMinLength  = p.suspensionMinLen;
        wheelTemplate.mSuspensionMaxLength  = p.suspensionMaxLen;
        // Пружина: критически демпфированная для реалистичного поведения
        wheelTemplate.mSuspensionSpring.mFrequency = p.suspensionFrequency;
        wheelTemplate.mSuspensionSpring.mDamping   = p.suspensionDamping;
        // Направление подвески — вниз
        wheelTemplate.mSuspensionDirection  = JPH::Vec3(0, -1, 0);
        wheelTemplate.mSteeringAxis         = JPH::Vec3(0,  1, 0);
        wheelTemplate.mWheelUp              = JPH::Vec3(0,  1, 0);
        wheelTemplate.mWheelForward         = JPH::Vec3(0,  0, 1);

        // mLongitudinalFriction — LinearCurve (slip ratio -> friction)
        // Стандартная кривая: при slip=0 friction=0, при slip=1 peak, потом чуть спад
        auto MakeFrictionCurve = [](float peak) {
            JPH::LinearCurve c;
            c.AddPoint(0.00f, 0.0f);
            c.AddPoint(0.06f, peak);
            c.AddPoint(0.20f, peak * 0.85f);
            c.AddPoint(1.00f, peak * 0.75f);
            return c;
        };
        // mLateralFriction — slip angle (радианы) -> friction
        // ОБЯЗАТЕЛЬНА: без неё Jolt assert'ит при первом контакте колеса с землёй
        auto MakeLateralCurve = [](float peak) {
            JPH::LinearCurve c;
            c.AddPoint(0.00f, 0.0f);
            c.AddPoint(0.05f, peak);
            c.AddPoint(0.20f, peak * 0.8f);
            c.AddPoint(1.00f, peak * 0.6f);
            return c;
        };
        JPH::LinearCurve frontFrictionCurve = MakeFrictionCurve(p.frontFriction);
        JPH::LinearCurve rearFrictionCurve  = MakeFrictionCurve(p.rearFriction);
        JPH::LinearCurve frontLateralCurve  = MakeLateralCurve(p.frontFriction);
        JPH::LinearCurve rearLateralCurve   = MakeLateralCurve(p.rearFriction);

        // FL
        JPH::WheelSettingsWV* wFL = new JPH::WheelSettingsWV(wheelTemplate);
        wFL->SetEmbedded();
        wFL->mPosition             = JPH::Vec3(-wX, wY, wZF);
        wFL->mMaxSteerAngle        = JPH::DegreesToRadians(p.maxSteerAngle);
        wFL->mLongitudinalFriction = frontFrictionCurve;
        wFL->mLateralFriction      = frontLateralCurve;

        // FR
        JPH::WheelSettingsWV* wFR = new JPH::WheelSettingsWV(wheelTemplate);
        wFR->SetEmbedded();
        wFR->mPosition             = JPH::Vec3( wX, wY, wZF);
        wFR->mMaxSteerAngle        = JPH::DegreesToRadians(p.maxSteerAngle);
        wFR->mLongitudinalFriction = frontFrictionCurve;
        wFR->mLateralFriction      = frontLateralCurve;

        // RL
        JPH::WheelSettingsWV* wRL = new JPH::WheelSettingsWV(wheelTemplate);
        wRL->SetEmbedded();
        wRL->mPosition             = JPH::Vec3(-wX, wY, wZR);
        wRL->mMaxSteerAngle        = 0.0f;
        wRL->mLongitudinalFriction = rearFrictionCurve;
        wRL->mLateralFriction      = rearLateralCurve;

        // RR
        JPH::WheelSettingsWV* wRR = new JPH::WheelSettingsWV(wheelTemplate);
        wRR->SetEmbedded();
        wRR->mPosition             = JPH::Vec3( wX, wY, wZR);
        wRR->mMaxSteerAngle        = 0.0f;
        wRR->mLongitudinalFriction = rearFrictionCurve;
        wRR->mLateralFriction      = rearLateralCurve;

        vcs.mWheels = { wFL, wFR, wRL, wRR };

        // ---- 4. WheeledVehicleController -----------------------------------
        JPH::WheeledVehicleControllerSettings* ctrl = new JPH::WheeledVehicleControllerSettings;
        ctrl->SetEmbedded();

        // Двигатель
        ctrl->mEngine.mMaxTorque           = p.maxTorque;
        ctrl->mEngine.mMaxRPM              = p.maxRPM;
        ctrl->mEngine.mInertia             = p.engineInertia;

        // Трансмиссия — автомат (1 передача вперёд, 1 назад)
        ctrl->mTransmission.mMode          = JPH::ETransmissionMode::Auto;
        ctrl->mTransmission.mGearRatios    = { p.gearRatio };
        ctrl->mTransmission.mReverseGearRatios = { -p.gearRatio * 0.7f };
        ctrl->mTransmission.mSwitchTime    = 0.5f;

        // Дифференциал: полный привод
        JPH::VehicleDifferentialSettings diff;
        diff.mLeftWheel                    = 0;   // FL
        diff.mRightWheel                   = 1;   // FR
        diff.mLimitedSlipRatio             = 1.4f;
        ctrl->mDifferentials.push_back(diff);
        diff.mLeftWheel                    = 2;   // RL
        diff.mRightWheel                   = 3;   // RR
        ctrl->mDifferentials.push_back(diff);

        // Антиролл-бары
        JPH::VehicleAntiRollBar arFront;
        arFront.mLeftWheel   = 0;
        arFront.mRightWheel  = 1;
        arFront.mStiffness   = p.antiRollFront;
        vcs.mAntiRollBars.push_back(arFront);

        JPH::VehicleAntiRollBar arRear;
        arRear.mLeftWheel    = 2;
        arRear.mRightWheel   = 3;
        arRear.mStiffness    = p.antiRollRear;
        vcs.mAntiRollBars.push_back(arRear);

        vcs.mController = ctrl;

        // ---- 5. VehicleConstraint создаём и сохраняем settings ----
        {
            JPH::BodyLockWrite lock(ph.physicsSystem->GetBodyLockInterface(), car.bodyID);
            JPH::Body& carBody = lock.GetBody();
            auto* vc = new JPH::VehicleConstraint(carBody, vcs);
            vc->SetEmbedded();  // ОБЯЗАТЕЛЬНО до присвоения в Ref<> — иначе refcount UB
            car.vehicleConstraint = vc;
        }
        auto* tester = new JPH::VehicleCollisionTesterRay(PhysLayers::STATIC);
        tester->SetEmbedded();
        car.vehicleConstraint->SetVehicleCollisionTester(tester);
        // AddConstraint + AddStepListener вызываются в SceneLoad::Run после OptimizeBroadPhase

        car.initialized = true;
        Logger::Info("Car body + VehicleConstraint created (not yet registered).");

#else
        car.initialized = true;
        Logger::Info("Car loaded (no physics build).");
        (void)ph;
#endif
    }

    void Destroy(CarState& car, PhysicsState& ph)
    {
#ifdef RK_JOLT_ENABLED
        if (!car.initialized || !ph.initialized) return;
        ph.physicsSystem->RemoveStepListener(car.vehicleConstraint);
        ph.physicsSystem->RemoveConstraint(car.vehicleConstraint);
        car.vehicleConstraint->Release();  // release Jolt ref before physicsSystem shutdown
        car.vehicleConstraint = nullptr;
        ph.bodyInterface->RemoveBody(car.bodyID);
        ph.bodyInterface->DestroyBody(car.bodyID);
#endif
        car.initialized = false;
    }
}
