#pragma once
// EngineAPI_Impl.h — заполняет EngineAPI реальными функциями движка.
// Живёт только внутри движка, сцена про него не знает.

#include "../../engine_api/EngineAPI.h"
#include "../physics/PhysicsState.h"
#include "../utils/Logger.h"

#ifdef RK_JOLT_ENABLED
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/Memory.h>
#include <Jolt/Core/IssueReporting.h>
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

    inline EngineAPI Build()
    {
        EngineAPI api;
        api.LogInfo           = LogInfo;
        api.LogWarn           = LogWarn;
        api.LogError          = LogError;
        api.SpawnStaticBox    = SpawnStaticBox;
        api.SpawnStaticBoxRot = SpawnStaticBoxRot;
        api.SpawnDynamicBox   = SpawnDynamicBox;
        api.DestroyBody       = DestroyBody;
        api.engineVersion     = 2;
        // ── Jolt синглтоны для InitJoltFromEngine() в DLL ────────────────
#ifdef RK_JOLT_ENABLED
        api.joltAllocate   = reinterpret_cast<void*>(JPH::Allocate);
        api.joltFree       = reinterpret_cast<void*>(JPH::Free);
        api.joltReallocate = reinterpret_cast<void*>(JPH::Reallocate);
        api.joltAllocate16 = reinterpret_cast<void*>(JPH::AlignedAllocate);
        api.joltFree16     = reinterpret_cast<void*>(JPH::AlignedFree);
        api.joltFactory    = reinterpret_cast<void*>(JPH::Factory::sInstance);
        api.joltAssertFn   = reinterpret_cast<void*>(JPH::AssertFailed);
#endif
        return api;
    }
}
