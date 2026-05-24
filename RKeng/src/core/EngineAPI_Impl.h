#pragma once
// EngineAPI_Impl.h — заполняет EngineAPI реальными функциями движка.
// Живёт только внутри движка, сцена про него не знает.

#include "../../engine_api/EngineAPI.h"
#include "../physics/PhysicsState.h"
#include "../utils/Logger.h"

#ifdef RK_JOLT_ENABLED
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/Memory.h>
#include <Jolt/Core/IssueReporting.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <cmath>
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

    // ── SpawnStaticBoxRot — статик с произвольной ротацией ───────────────────
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

    // ── SpawnDynamicBox — динамическое тело (кузов машины) ───────────────────
    // BoxShapeSettings::Create() вызывается здесь, в RKengCore — Factory
    // гарантированно инициализирован. DLL-плагину линковать Jolt не нужно.
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


    // ── GetBodyTransform — позиция/ротация тела без BodyInterface.h в DLL ──
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

    // ── Персонаж: velocity bridge — безопасная альтернатива CharacterVirtual.h в DLL ──
    // JPH_IMPLEMENT_RTTI_VIRTUAL в CharacterVirtual.h создаёт глобальные объекты
    // при загрузке DLL → краш до DllMain. DLL использует эти три API-функции.
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


    // ── CreateCharacter — создаёт CharacterVirtual по запросу DLL-сцены ─────
    // DLL не включает CharacterVirtual.h (JPH_IMPLEMENT_RTTI_VIRTUAL → краш).
    // Вызывать после SpawnStaticBox + OptimizeBroadPhase.
    static bool CreateCharacter(PhysicsState& ph, const RK_CharacterDesc& desc)
    {
#ifdef RK_JOLT_ENABLED
        if (!ph.initialized || !ph.physicsSystem) return false;
        if (ph.character) return true; // уже создан

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

    inline EngineAPI Build()
    {
        EngineAPI api;
        api.LogInfo           = LogInfo;
        api.LogWarn           = LogWarn;
        api.LogError          = LogError;
        api.SpawnStaticBox    = SpawnStaticBox;
        api.SpawnStaticBoxRot = SpawnStaticBoxRot;
        api.SpawnDynamicBox   = SpawnDynamicBox;
        api.DestroyBody          = DestroyBody;
        api.GetBodyTransform      = GetBodyTransform;
        api.SetPlayerVelocity    = SetPlayerVelocity;
        api.GetPlayerVelocity    = GetPlayerVelocity;
        api.GetGravityY          = GetGravityY;
        api.CreateCharacter       = CreateCharacter;
        api.engineVersion        = 4;
        // ── Jolt синглтоны для InitJoltFromEngine() в DLL ────────────────
#ifdef RK_JOLT_ENABLED
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
