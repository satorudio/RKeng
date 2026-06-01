#pragma once
// EngineAPI_Impl.h — заполняет EngineAPI реальными функциями движка.
// Живёт только внутри движка, сцена про него не знает.

#include "../../engine_api/EngineAPI.h"
#include "../physics/PhysicsState.h"
#include "../utils/Logger.h"

#ifdef RK_JOLT_ENABLED
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyLockInterface.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#include <Jolt/Physics/Vehicle/VehicleCollisionTester.h>
#include <Jolt/Physics/Vehicle/WheeledVehicleController.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/Memory.h>
#include <Jolt/Core/IssueReporting.h>
#include <cmath>
#include <string>
#endif

namespace RKeng::EngineAPI_Impl
{
    static void LogInfo (const char* msg) { Logger::Info (msg); }
    static void LogWarn (const char* msg) { Logger::Warn (msg); }
    static void LogError(const char* msg) { Logger::Error(msg); }

    // ── Legacy SpawnStaticBox (без ротации) ───────────────────────────────────
    static uint32_t SpawnStaticBox(PhysicsState& ph, const RK_BoxBody& box)
    {
#ifdef RK_JOLT_ENABLED
        if (!ph.bodyInterface) return UINT32_MAX;
        JPH::BoxShapeSettings ss(
            JPH::Vec3(box.halfExtents.x, box.halfExtents.y, box.halfExtents.z));
        auto r = ss.Create();
        if (r.HasError()) return UINT32_MAX;
        JPH::BodyCreationSettings bcs(
            r.Get(),
            JPH::RVec3(box.position.x, box.position.y, box.position.z),
            JPH::Quat::sIdentity(),
            JPH::EMotionType::Static,
            PhysLayers::STATIC);
        JPH::Body* body = ph.bodyInterface->CreateBody(bcs);
        if (!body) return UINT32_MAX;
        ph.bodyInterface->AddBody(body->GetID(), JPH::EActivation::DontActivate);
        return body->GetID().GetIndexAndSequenceNumber();
#else
        (void)ph; (void)box; return UINT32_MAX;
#endif
    }

    // ── SpawnStaticBoxRot ─────────────────────────────────────────────────────
    static uint32_t SpawnStaticBoxRot(PhysicsState& ph, const RK_StaticBox& box)
    {
#ifdef RK_JOLT_ENABLED
        if (!ph.bodyInterface) return UINT32_MAX;
        JPH::BoxShapeSettings ss(JPH::Vec3(box.hx, box.hy, box.hz));
        ss.SetEmbedded();
        auto result = ss.Create();
        if (result.HasError()) {
            Logger::Error("EngineAPI::SpawnStaticBoxRot: shape creation failed");
            return UINT32_MAX;
        }
        JPH::Quat rot =
            JPH::Quat::sRotation(JPH::Vec3::sAxisY(), box.rotY) *
            JPH::Quat::sRotation(JPH::Vec3::sAxisX(), box.rotX);
        JPH::BodyCreationSettings bcs(
            result.Get(),
            JPH::RVec3(box.cx, box.cy, box.cz),
            rot,
            JPH::EMotionType::Static,
            PhysLayers::STATIC);
        JPH::BodyID id = ph.bodyInterface->CreateAndAddBody(
            bcs, JPH::EActivation::DontActivate);
        return id.GetIndexAndSequenceNumber();
#else
        (void)ph; (void)box; return UINT32_MAX;
#endif
    }

    // ── SpawnDynamicBox ───────────────────────────────────────────────────────
    static uint32_t SpawnDynamicBox(PhysicsState& ph, const RK_DynamicBox& box)
    {
#ifdef RK_JOLT_ENABLED
        if (!ph.bodyInterface) return UINT32_MAX;
        JPH::BoxShapeSettings ss(JPH::Vec3(box.hx, box.hy, box.hz));
        ss.SetEmbedded();
        auto result = ss.Create();
        if (result.HasError()) {
            Logger::Error("EngineAPI::SpawnDynamicBox: shape creation failed");
            return UINT32_MAX;
        }
        JPH::BodyCreationSettings bcs(
            result.Get(),
            JPH::RVec3(box.cx, box.cy, box.cz),
            JPH::Quat::sIdentity(),
            JPH::EMotionType::Dynamic,
            PhysLayers::DYNAMIC);
        bcs.mOverrideMassProperties       = JPH::EOverrideMassProperties::CalculateInertia;
        bcs.mMassPropertiesOverride.mMass = box.mass;
        bcs.mLinearDamping                = box.linearDamping;
        bcs.mAngularDamping               = box.angularDamping;
        bcs.mFriction                     = box.friction;
        JPH::BodyID id = ph.bodyInterface->CreateAndAddBody(
            bcs, JPH::EActivation::Activate);
        return id.GetIndexAndSequenceNumber();
#else
        (void)ph; (void)box; return UINT32_MAX;
#endif
    }

    // ── GetBodyTransform ──────────────────────────────────────────────────────
    static bool GetBodyTransform(PhysicsState& ph, uint32_t bodyID,
        float& px, float& py, float& pz,
        float& qx, float& qy, float& qz, float& qw)
    {
#ifdef RK_JOLT_ENABLED
        if (!ph.bodyInterface) return false;
        JPH::BodyID jid(bodyID);
        if (!ph.bodyInterface->IsAdded(jid)) return false;
        JPH::RVec3 p = ph.bodyInterface->GetPosition(jid);
        JPH::Quat  q = ph.bodyInterface->GetRotation(jid);
        px = (float)p.GetX(); py = (float)p.GetY(); pz = (float)p.GetZ();
        qx = q.GetX(); qy = q.GetY(); qz = q.GetZ(); qw = q.GetW();
        return true;
#else
        (void)ph; (void)bodyID;
        px=py=pz=qx=qy=qz=0.f; qw=1.f;
        return false;
#endif
    }

    // ── Персонаж ──────────────────────────────────────────────────────────────
    static void SetPlayerVelocity(PhysicsState& ph, float vx, float vy, float vz)
    {
#ifdef RK_JOLT_ENABLED
        if (ph.character)
            ph.character->SetLinearVelocity(JPH::Vec3(vx, vy, vz));
#else
        (void)ph; (void)vx; (void)vy; (void)vz;
#endif
    }

    static void GetPlayerVelocity(PhysicsState& ph, float& vx, float& vy, float& vz)
    {
#ifdef RK_JOLT_ENABLED
        if (ph.character) {
            JPH::Vec3 v = ph.character->GetLinearVelocity();
            vx = v.GetX(); vy = v.GetY(); vz = v.GetZ();
        } else { vx = vy = vz = 0.0f; }
#else
        (void)ph; vx = vy = vz = 0.0f;
#endif
    }

    static float GetGravityY(PhysicsState& ph)
    {
#ifdef RK_JOLT_ENABLED
        if (ph.physicsSystem)
            return ph.physicsSystem->GetGravity().GetY();
#else
        (void)ph;
#endif
        return -9.81f;
    }

    static void DestroyBody(PhysicsState& ph, uint32_t bodyID)
    {
#ifdef RK_JOLT_ENABLED
        if (!ph.bodyInterface) return;
        JPH::BodyID jid(bodyID);
        ph.bodyInterface->RemoveBody(jid);
        ph.bodyInterface->DestroyBody(jid);
#else
        (void)ph; (void)bodyID;
#endif
    }

    static bool CreateCharacter(PhysicsState& ph, const RK_CharacterDesc& desc)
    {
#ifdef RK_JOLT_ENABLED
        if (!ph.initialized || !ph.physicsSystem) return false;
        if (ph.character) return true;

        JPH::CharacterVirtualSettings cs;
        cs.mMaxSlopeAngle             = JPH::DegreesToRadians(desc.maxSlopeAngleDeg);
        cs.mMaxStrength               = 100.0f;
        cs.mBackFaceMode              = JPH::EBackFaceMode::CollideWithBackFaces;
        cs.mCharacterPadding          = 0.02f;
        cs.mPenetrationRecoverySpeed  = 1.0f;
        cs.mPredictiveContactDistance = 0.1f;

        JPH::CapsuleShapeSettings capSS(desc.capsuleHalfHeight, desc.capsuleRadius);
        auto capResult = capSS.Create();
        if (capResult.HasError()) {
            Logger::Error(std::string("[API] CreateCharacter capsule error: ") +
                          capResult.GetError().c_str());
            return false;
        }

        JPH::RotatedTranslatedShapeSettings rtsSS(
            JPH::Vec3(0.f, desc.capsuleHalfHeight + desc.capsuleRadius, 0.f),
            JPH::Quat::sIdentity(),
            capResult.Get());
        auto rtsResult = rtsSS.Create();
        if (rtsResult.HasError()) {
            Logger::Error("[API] CreateCharacter RotatedTranslatedShape error");
            return false;
        }
        cs.mShape = rtsResult.Get();

        ph.character = std::make_unique<JPH::CharacterVirtual>(
            &cs,
            JPH::RVec3(desc.spawnX, desc.spawnY, desc.spawnZ),
            JPH::Quat::sIdentity(),
            ph.physicsSystem.get());

        Logger::Info("[API] CharacterVirtual created at ("
                     + std::to_string(desc.spawnX) + ","
                     + std::to_string(desc.spawnY) + ","
                     + std::to_string(desc.spawnZ) + ")");
        return true;
#else
        (void)ph; (void)desc; return false;
#endif
    }

    // ── Транспорт (версия API 6) ──────────────────────────────────────────────
    // VehicleConstraint создаётся здесь, в движке, где Factory гарантирован.
    // DLL не линкует Jolt и не включает VehicleConstraint.h.

#ifdef RK_JOLT_ENABLED
    static constexpr uint32_t MAX_VEHICLES = 16;

    struct VehicleEntry
    {
        JPH::VehicleConstraint* constraint = nullptr;
        JPH::BodyID             bodyID;
    };

    static VehicleEntry s_vehicles[MAX_VEHICLES];

    static JPH::LinearCurve MakeFrictionCurve(float peak)
    {
        JPH::LinearCurve c;
        c.AddPoint(0.00f, 0.0f);
        c.AddPoint(0.06f, peak);
        c.AddPoint(0.20f, peak * 0.85f);
        c.AddPoint(1.00f, peak * 0.70f);
        return c;
    }
#endif

    static RK_VehicleHandle SpawnVehicle(PhysicsState& ph, const RK_VehicleDesc& d)
    {
#ifdef RK_JOLT_ENABLED
        if (!ph.initialized || !ph.bodyInterface || !ph.physicsSystem) {
            Logger::Error("[SpawnVehicle] PhysicsState not ready");
            return RK_INVALID_VEHICLE;
        }

        // 1. Кузов
        JPH::BoxShapeSettings bss(JPH::Vec3(d.halfW, d.halfH, d.halfL));
        bss.SetEmbedded();
        auto bsResult = bss.Create();
        if (bsResult.HasError()) {
            Logger::Error("[SpawnVehicle] BoxShape error");
            return RK_INVALID_VEHICLE;
        }
        JPH::BodyCreationSettings bcs(
            bsResult.Get(),
            JPH::RVec3(d.spawnX, d.spawnY, d.spawnZ),
            JPH::Quat::sIdentity(),
            JPH::EMotionType::Dynamic,
            PhysLayers::DYNAMIC);
        bcs.mOverrideMassProperties       = JPH::EOverrideMassProperties::CalculateInertia;
        bcs.mMassPropertiesOverride.mMass = d.mass;
        bcs.mLinearDamping                = d.linearDamping;
        bcs.mAngularDamping               = d.angularDamping;
        bcs.mFriction                     = d.bodyFriction;
        JPH::BodyID bodyID = ph.bodyInterface->CreateAndAddBody(
            bcs, JPH::EActivation::Activate);
        if (bodyID.IsInvalid()) {
            Logger::Error("[SpawnVehicle] CreateAndAddBody failed");
            return RK_INVALID_VEHICLE;
        }

        // 2. VehicleConstraintSettings
        JPH::VehicleConstraintSettings vcs;
        vcs.mUp      = JPH::Vec3::sAxisY();
        vcs.mForward = JPH::Vec3::sAxisZ();

        // 3. Колёса
        const float wX  = d.halfW + d.wheelWidth * 0.4f + 0.02f;
        const float wY  = -(d.halfH + d.suspMaxLen * 0.6f);
        const float wZF =  d.halfL * 0.70f;
        const float wZR = -d.halfL * 0.70f;

        JPH::WheelSettingsWV tmpl;
        tmpl.mRadius              = d.wheelRadius;
        tmpl.mWidth               = d.wheelWidth;
        tmpl.mSuspensionMinLength = d.suspMinLen;
        tmpl.mSuspensionMaxLength = d.suspMaxLen;
        tmpl.mSuspensionSpring.mFrequency = d.suspFreq;
        tmpl.mSuspensionSpring.mDamping   = d.suspDamping;
        tmpl.mSuspensionDirection = JPH::Vec3(0, -1, 0);
        tmpl.mSteeringAxis        = JPH::Vec3(0,  1, 0);
        tmpl.mWheelUp             = JPH::Vec3(0,  1, 0);
        tmpl.mWheelForward        = JPH::Vec3(0,  0, 1);

        auto fLon = MakeFrictionCurve(d.frontFriction);
        auto rLon = MakeFrictionCurve(d.rearFriction);
        auto fLat = MakeFrictionCurve(d.frontFriction);
        auto rLat = MakeFrictionCurve(d.rearFriction);

        auto MakeWheel = [&](float wx, float wz, float steerDeg,
                             const JPH::LinearCurve& lon,
                             const JPH::LinearCurve& lat) -> JPH::WheelSettingsWV*
        {
            auto* w = new JPH::WheelSettingsWV(tmpl);
            w->SetEmbedded();
            w->mPosition             = JPH::Vec3(wx, wY, wz);
            w->mMaxSteerAngle        = JPH::DegreesToRadians(steerDeg);
            w->mLongitudinalFriction = lon;
            w->mLateralFriction      = lat;
            return w;
        };

        vcs.mWheels = {
            MakeWheel(-wX, wZF, d.maxSteerDeg, fLon, fLat),  // FL
            MakeWheel( wX, wZF, d.maxSteerDeg, fLon, fLat),  // FR
            MakeWheel(-wX, wZR, 0.f,           rLon, rLat),  // RL
            MakeWheel( wX, wZR, 0.f,           rLon, rLat),  // RR
        };

        // 4. Контроллер
        auto* ctrl = new JPH::WheeledVehicleControllerSettings;
        ctrl->SetEmbedded();
        ctrl->mEngine.mMaxTorque = d.maxTorque;
        ctrl->mEngine.mMaxRPM    = d.maxRPM;
        ctrl->mEngine.mInertia   = d.engineInertia;
        ctrl->mTransmission.mMode = JPH::ETransmissionMode::Auto;
        ctrl->mTransmission.mGearRatios        = { 4.0f, 2.5f, 1.7f, 1.2f, 1.0f, 0.8f };
        ctrl->mTransmission.mReverseGearRatios = { -3.5f };
        ctrl->mTransmission.mSwitchTime = 0.3f;

        JPH::VehicleDifferentialSettings diff;
        diff.mLeftWheel = 2; diff.mRightWheel = 3;
        diff.mLimitedSlipRatio = 1.4f;
        ctrl->mDifferentials.push_back(diff);

        JPH::VehicleAntiRollBar arF; arF.mLeftWheel=0; arF.mRightWheel=1; arF.mStiffness=d.antiRollFront;
        JPH::VehicleAntiRollBar arR; arR.mLeftWheel=2; arR.mRightWheel=3; arR.mStiffness=d.antiRollRear;
        vcs.mAntiRollBars = { arF, arR };
        vcs.mController = ctrl;

        // 5. VehicleConstraint — создаётся внутри движка, Factory гарантирован
        // BodyLockRead берём только чтобы получить Body&.
        // VehicleConstraint создаём после освобождения лока —
        // его конструктор сам берёт внутренние локи.
        JPH::Body* bodyPtr = nullptr;
        {
            JPH::BodyLockRead lock(ph.physicsSystem->GetBodyLockInterface(), bodyID);
            if (!lock.Succeeded()) {
                Logger::Error("[SpawnVehicle] BodyLockRead failed — invalid bodyID");
                ph.bodyInterface->RemoveBody(bodyID);
                ph.bodyInterface->DestroyBody(bodyID);
                return RK_INVALID_VEHICLE;
            }
            bodyPtr = const_cast<JPH::Body*>(&lock.GetBody());
        } // лок отпущен; bodyPtr остаётся валидным (тело живёт в PhysicsSystem)

        auto* vc = new JPH::VehicleConstraint(*bodyPtr, vcs);
        vc->SetEmbedded();

        auto* tester = new JPH::VehicleCollisionTesterRay(PhysLayers::DYNAMIC);
        tester->SetEmbedded();
        vc->SetVehicleCollisionTester(tester);

        ph.physicsSystem->AddConstraint(vc);
        ph.physicsSystem->AddStepListener(vc);

        // 6. Регистрация
        for (uint32_t i = 0; i < MAX_VEHICLES; ++i) {
            if (s_vehicles[i].constraint == nullptr) {
                s_vehicles[i] = { vc, bodyID };
                Logger::Info("[SpawnVehicle] handle=" + std::to_string(i));
                return i;
            }
        }

        Logger::Error("[SpawnVehicle] vehicle registry full");
        ph.physicsSystem->RemoveStepListener(vc);
        ph.physicsSystem->RemoveConstraint(vc);
        vc->Release();
        ph.bodyInterface->RemoveBody(bodyID);
        ph.bodyInterface->DestroyBody(bodyID);
        return RK_INVALID_VEHICLE;
#else
        (void)ph; (void)d;
        return RK_INVALID_VEHICLE;
#endif
    }

    static void SetVehicleInput(PhysicsState& ph, RK_VehicleHandle vh,
                                const RK_VehicleInput& inp)
    {
#ifdef RK_JOLT_ENABLED
        (void)ph;
        if (vh >= MAX_VEHICLES || !s_vehicles[vh].constraint) return;
        auto* ctrl = static_cast<JPH::WheeledVehicleController*>(
            s_vehicles[vh].constraint->GetController());
        // Jolt WheeledVehicleController::SetDriverInput(inForward, inBrake, inRight, inHandBrake)
        // throttle=forward, brake=brake, steer=right — порядок подтверждён оригинальным CarTick движка
        ctrl->SetDriverInput(inp.throttle, inp.brake, inp.steer, inp.handbrake);
#else
        (void)ph; (void)vh; (void)inp;
#endif
    }

    static bool GetVehicleTransform(PhysicsState& ph, RK_VehicleHandle vh,
                                    float& px, float& py, float& pz,
                                    float& qx, float& qy, float& qz, float& qw,
                                    float& vx, float& vy, float& vz)
    {
#ifdef RK_JOLT_ENABLED
        if (vh >= MAX_VEHICLES || !s_vehicles[vh].constraint || !ph.bodyInterface) {
            px=py=pz=qx=qy=qz=0.f; qw=1.f; vx=vy=vz=0.f;
            return false;
        }
        JPH::BodyID bid = s_vehicles[vh].bodyID;
        JPH::RVec3 p = ph.bodyInterface->GetPosition(bid);
        JPH::Quat  q = ph.bodyInterface->GetRotation(bid);
        JPH::Vec3  vel = ph.bodyInterface->GetLinearVelocity(bid);
        px=(float)p.GetX(); py=(float)p.GetY(); pz=(float)p.GetZ();
        qx=q.GetX(); qy=q.GetY(); qz=q.GetZ(); qw=q.GetW();
        vx=vel.GetX(); vy=vel.GetY(); vz=vel.GetZ();
        return true;
#else
        (void)ph; (void)vh;
        px=py=pz=qx=qy=qz=0.f; qw=1.f; vx=vy=vz=0.f;
        return false;
#endif
    }

    static void DestroyVehicle(PhysicsState& ph, RK_VehicleHandle vh)
    {
#ifdef RK_JOLT_ENABLED
        if (vh >= MAX_VEHICLES || !s_vehicles[vh].constraint) return;
        auto& e = s_vehicles[vh];
        if (ph.physicsSystem) {
            ph.physicsSystem->RemoveStepListener(e.constraint);
            ph.physicsSystem->RemoveConstraint(e.constraint);
        }
        e.constraint->Release();
        if (ph.bodyInterface && !e.bodyID.IsInvalid()) {
            ph.bodyInterface->RemoveBody(e.bodyID);
            ph.bodyInterface->DestroyBody(e.bodyID);
        }
        e = {};
        Logger::Info("[DestroyVehicle] handle=" + std::to_string(vh));
#else
        (void)ph; (void)vh;
#endif
    }

    inline EngineAPI Build()
    {
        EngineAPI api;
        // Логгер — без PhysicsState, подключаем напрямую
        api.LogInfo           = LogInfo;
        api.LogWarn           = LogWarn;
        api.LogError          = LogError;
        // Все функции-имплементации принимают PhysicsState& внутри движка.
        // Снаружи API принимает RK_WorldHandle (uint32_t).
        // Адаптеры ниже игнорируют world (он всегда 0) и берут глобальный PhysicsState.
        api.SpawnStaticBox    = [](RK_WorldHandle, const RK_BoxBody& b)    { return SpawnStaticBox   (GetPhysicsState(), b); };
        api.SpawnStaticBoxRot = [](RK_WorldHandle, const RK_StaticBox& b)  { return SpawnStaticBoxRot(GetPhysicsState(), b); };
        api.SpawnDynamicBox   = [](RK_WorldHandle, const RK_DynamicBox& b) { return SpawnDynamicBox  (GetPhysicsState(), b); };
        api.DestroyBody       = [](RK_WorldHandle, uint32_t id)            { DestroyBody    (GetPhysicsState(), id); };
        api.GetBodyTransform  = [](RK_WorldHandle, uint32_t id,
                                   float& px, float& py, float& pz,
                                   float& qx, float& qy, float& qz, float& qw)
                                { return GetBodyTransform(GetPhysicsState(), id, px, py, pz, qx, qy, qz, qw); };
        api.SetPlayerVelocity = [](RK_WorldHandle, float vx, float vy, float vz)
                                { SetPlayerVelocity(GetPhysicsState(), vx, vy, vz); };
        api.GetPlayerVelocity = [](RK_WorldHandle, float& vx, float& vy, float& vz)
                                { GetPlayerVelocity(GetPhysicsState(), vx, vy, vz); };
        api.GetGravityY       = [](RK_WorldHandle)       { return GetGravityY    (GetPhysicsState()); };
        api.CreateCharacter   = [](RK_WorldHandle, const RK_CharacterDesc& d)  { return CreateCharacter(GetPhysicsState(), d); };
        // Транспорт (v6)
        api.SpawnVehicle      = [](RK_WorldHandle, const RK_VehicleDesc& d)    { return SpawnVehicle   (GetPhysicsState(), d); };
        api.SetVehicleInput   = [](RK_WorldHandle, RK_VehicleHandle vh, const RK_VehicleInput& i)
                                { SetVehicleInput(GetPhysicsState(), vh, i); };
        api.GetVehicleTransform = [](RK_WorldHandle, RK_VehicleHandle vh,
                                     float& px, float& py, float& pz,
                                     float& qx, float& qy, float& qz, float& qw,
                                     float& vx, float& vy, float& vz)
                                  { return GetVehicleTransform(GetPhysicsState(), vh, px, py, pz, qx, qy, qz, qw, vx, vy, vz); };
        api.DestroyVehicle    = [](RK_WorldHandle, RK_VehicleHandle vh) { DestroyVehicle(GetPhysicsState(), vh); };
        api.OptimizeBroadPhase = [](RK_WorldHandle) {
#ifdef RK_JOLT_ENABLED
            auto& ph = GetPhysicsState();
            if (ph.initialized && ph.physicsSystem)
                ph.physicsSystem->OptimizeBroadPhase();
#endif
        };
        api.engineVersion        = 8;
#ifdef RK_JOLT_ENABLED
        // Jolt-синглтоны (v5, оставлены для обратной совместимости)
        api.joltAllocate   = reinterpret_cast<void*>(JPH::Allocate);
        api.joltFree       = reinterpret_cast<void*>(JPH::Free);
        api.joltReallocate = reinterpret_cast<void*>(JPH::Reallocate);
        api.joltAllocate16 = reinterpret_cast<void*>(JPH::AlignedAllocate);
        api.joltFree16     = reinterpret_cast<void*>(JPH::AlignedFree);
        api.joltFactory    = reinterpret_cast<void*>(JPH::Factory::sInstance);
        api.joltAssertFn   = reinterpret_cast<void*>(JPH::AssertFailed);
        api.joltTrace      = reinterpret_cast<void*>(JPH::Trace);
#endif
        return api;
    }
}