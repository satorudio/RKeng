// CarLoad.cpp — создаёт box-кузов через EngineAPI + VehicleConstraint напрямую.
// Правило: кузов ТОЛЬКО через api.SpawnDynamicBox.
//          VehicleConstraint — напрямую (не трогает Factory).

#include "CarLoad.h"
#include "CarMesh.h"

#ifdef RK_JOLT_ENABLED
#  include <Jolt/Jolt.h>
#  include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#  include <Jolt/Physics/Vehicle/VehicleCollisionTester.h>
#  include <Jolt/Physics/Vehicle/WheeledVehicleController.h>
#  include <Jolt/Physics/Body/BodyLockInterface.h>
#  include <Jolt/Physics/Body/Body.h>
#endif

namespace RKeng::CarLoad
{
#ifdef RK_JOLT_ENABLED
    static JPH::LinearCurve MakeCurve(float peak)
    {
        JPH::LinearCurve c;
        c.AddPoint(0.00f, 0.0f);
        c.AddPoint(0.06f, peak);
        c.AddPoint(0.20f, peak * 0.85f);
        c.AddPoint(1.00f, peak * 0.70f);
        return c;
    }
#endif

    void Run(CarState& car, PhysicsState& ph, Vec3 spawnPos, const EngineAPI& api)
    {
        car.position    = spawnPos;
        car.orientation = Quat(1,0,0,0);
        car.meshDirty   = true;

#ifdef RK_JOLT_ENABLED
        if (!ph.initialized) { if (api.LogError) api.LogError("CarLoad: physics not init"); return; }
        if (!api.SpawnDynamicBox) { if (api.LogError) api.LogError("CarLoad: no SpawnDynamicBox"); return; }

        const auto& p = car.params;

        // ── 1. Кузов ─────────────────────────────────────────────────────
        RK_DynamicBox bd{};
        bd.cx = spawnPos.x; bd.cy = spawnPos.y; bd.cz = spawnPos.z;
        bd.hx = p.halfW; bd.hy = p.halfH; bd.hz = p.halfL;
        bd.mass = p.mass;
        bd.linearDamping  = p.linearDamping;
        bd.angularDamping = p.angularDamping;
        bd.friction       = p.friction;

        if (api.LogInfo) api.LogInfo("CarLoad: calling SpawnDynamicBox");
        uint32_t rawID = api.SpawnDynamicBox(ph, bd);
        if (api.LogInfo) api.LogInfo("CarLoad: SpawnDynamicBox returned");
        if (rawID == UINT32_MAX) { if (api.LogError) api.LogError("CarLoad: SpawnDynamicBox failed"); return; }
        car.bodyID = JPH::BodyID(rawID);
        if (api.LogInfo) api.LogInfo("CarLoad: bodyID set");

        // ── 2. VehicleConstraintSettings ─────────────────────────────────
        if (api.LogInfo) api.LogInfo("CarLoad: creating VehicleConstraintSettings");
        JPH::VehicleConstraintSettings vcs;
        vcs.mUp      = JPH::Vec3::sAxisY();
        vcs.mForward = JPH::Vec3::sAxisZ();

        // ── 3. Колёса ────────────────────────────────────────────────────
        const float wX  = p.halfW + p.wheelWidth * 0.4f + 0.02f;
        const float wY  = -(p.halfH + p.suspMaxLen * 0.6f);
        const float wZF =  p.halfL * 0.70f;
        const float wZR = -p.halfL * 0.70f;

        JPH::WheelSettingsWV tmpl;
        tmpl.mRadius              = p.wheelRadius;
        tmpl.mWidth               = p.wheelWidth;
        tmpl.mSuspensionMinLength = p.suspMinLen;
        tmpl.mSuspensionMaxLength = p.suspMaxLen;
        tmpl.mSuspensionSpring.mFrequency = p.suspFreq;
        tmpl.mSuspensionSpring.mDamping   = p.suspDamping;
        tmpl.mSuspensionDirection = JPH::Vec3(0,-1,0);
        tmpl.mSteeringAxis        = JPH::Vec3(0, 1,0);
        tmpl.mWheelUp             = JPH::Vec3(0, 1,0);
        tmpl.mWheelForward        = JPH::Vec3(0, 0,1);

        auto fLon = MakeCurve(p.frontFriction);
        auto rLon = MakeCurve(p.rearFriction);
        auto fLat = MakeCurve(p.frontFriction);
        auto rLat = MakeCurve(p.rearFriction);

        auto MakeWheel = [&](float wx, float wz, float steer,
                             const JPH::LinearCurve& lon,
                             const JPH::LinearCurve& lat) -> JPH::WheelSettingsWV*
        {
            auto* w = new JPH::WheelSettingsWV(tmpl);
            w->SetEmbedded();
            w->mPosition             = JPH::Vec3(wx, wY, wz);
            w->mMaxSteerAngle        = JPH::DegreesToRadians(steer);
            w->mLongitudinalFriction = lon;
            w->mLateralFriction      = lat;
            return w;
        };

        // 0=FL, 1=FR, 2=RL, 3=RR
        vcs.mWheels = {
            MakeWheel(-wX, wZF, p.maxSteerDeg, fLon, fLat),
            MakeWheel( wX, wZF, p.maxSteerDeg, fLon, fLat),
            MakeWheel(-wX, wZR, 0.0f,          rLon, rLat),
            MakeWheel( wX, wZR, 0.0f,          rLon, rLat),
        };

        // ── 4. Контроллер ────────────────────────────────────────────────
        auto* ctrl = new JPH::WheeledVehicleControllerSettings;
        ctrl->SetEmbedded();
        ctrl->mEngine.mMaxTorque = p.maxTorque;
        ctrl->mEngine.mMaxRPM    = p.maxRPM;
        ctrl->mEngine.mInertia   = p.engineInertia;
        ctrl->mTransmission.mMode = JPH::ETransmissionMode::Auto;
        ctrl->mTransmission.mGearRatios        = { 4.0f, 2.5f, 1.7f, 1.2f, 1.0f, 0.8f };
        ctrl->mTransmission.mReverseGearRatios = { -3.5f };
        ctrl->mTransmission.mSwitchTime = 0.3f;

        // RWD дифф
        JPH::VehicleDifferentialSettings diff;
        diff.mLeftWheel = 2; diff.mRightWheel = 3;
        diff.mLimitedSlipRatio = 1.4f;
        ctrl->mDifferentials.push_back(diff);

        // Антикрен
        JPH::VehicleAntiRollBar arF; arF.mLeftWheel=0; arF.mRightWheel=1; arF.mStiffness=p.antiRollFront;
        JPH::VehicleAntiRollBar arR; arR.mLeftWheel=2; arR.mRightWheel=3; arR.mStiffness=p.antiRollRear;
        vcs.mAntiRollBars = { arF, arR };
        vcs.mController = ctrl;

        // ── 5. VehicleConstraint ──────────────────────────────────────────
        if (api.LogInfo) api.LogInfo("CarLoad: creating VehicleConstraint");
        {
            JPH::BodyLockWrite lock(ph.physicsSystem->GetBodyLockInterface(), car.bodyID);
            if (api.LogInfo) api.LogInfo("CarLoad: lock acquired");
            JPH::Body& body = lock.GetBody();
            if (api.LogInfo) api.LogInfo("CarLoad: body acquired");
            auto* vc = new JPH::VehicleConstraint(body, vcs);
            vc->SetEmbedded();
            car.vehicleConstraint = vc;
        }

        auto* tester = new JPH::VehicleCollisionTesterRay(PhysLayers::STATIC);
        tester->SetEmbedded();
        car.vehicleConstraint->SetVehicleCollisionTester(tester);

        car.initialized = true;
        if (api.LogInfo) api.LogInfo("CarLoad: OK");
#else
        (void)ph;
        car.initialized = true;
#endif
    }

    void Destroy(CarState& car, PhysicsState& ph)
    {
#ifdef RK_JOLT_ENABLED
        if (!car.initialized || !ph.initialized || !ph.physicsSystem) return;
        if (car.vehicleConstraint) {
            ph.physicsSystem->RemoveStepListener(car.vehicleConstraint);
            ph.physicsSystem->RemoveConstraint(car.vehicleConstraint);
            car.vehicleConstraint->Release();
            car.vehicleConstraint = nullptr;
        }
        if (!car.bodyID.IsInvalid()) {
            ph.bodyInterface->RemoveBody(car.bodyID);
            ph.bodyInterface->DestroyBody(car.bodyID);
        }
#else
        (void)ph;
#endif
        car.initialized = false;
    }
}
