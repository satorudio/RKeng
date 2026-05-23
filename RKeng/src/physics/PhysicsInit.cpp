#include "PhysicsInit.h"
#include "../utils/Logger.h"
#include <cstdio>
#include <cstdarg>

#ifdef RK_JOLT_ENABLED

#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Core/IssueReporting.h>

// ---- Jolt диагностика: пишем trace в лог ----
static void JoltTraceImpl(const char* fmt, ...)
{
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    RKeng::Logger::Info(std::string("[Jolt] ") + buf);
}

// JPH::AssertFailed определяется самим Jolt (IssueReporting.cpp) начиная с v5.
// Не переопределяем — иначе multiple definition при линковке.


class RKBPLayerInterface final : public JPH::BroadPhaseLayerInterface
{
public:
    JPH::uint GetNumBroadPhaseLayers() const override { return RKeng::BPLayers::COUNT; }
    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override
    {
        return (layer == RKeng::PhysLayers::STATIC)
            ? RKeng::BPLayers::NON_MOVING : RKeng::BPLayers::MOVING;
    }
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override
    { return (layer == RKeng::BPLayers::NON_MOVING) ? "NON_MOVING" : "MOVING"; }
#endif
};

class RKObjVsBPFilter final : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    bool ShouldCollide(JPH::ObjectLayer obj, JPH::BroadPhaseLayer bp) const override
    {
        if (obj == RKeng::PhysLayers::STATIC)  return bp == RKeng::BPLayers::MOVING;
        if (obj == RKeng::PhysLayers::DYNAMIC) return true;
        return false;
    }
};

class RKObjPairFilter final : public JPH::ObjectLayerPairFilter
{
public:
    bool ShouldCollide(JPH::ObjectLayer a, JPH::ObjectLayer b) const override
    {
        if (a == RKeng::PhysLayers::STATIC)  return b == RKeng::PhysLayers::DYNAMIC;
        if (a == RKeng::PhysLayers::DYNAMIC) return true;
        return false;
    }
};

static RKBPLayerInterface  s_BPLayerInterface;
static RKObjVsBPFilter     s_ObjVsBPFilter;
static RKObjPairFilter     s_ObjPairFilter;

#endif // RK_JOLT_ENABLED

namespace RKeng::PhysicsInit
{
    void Run(PhysicsState& ph)
    {
#ifdef RK_JOLT_ENABLED
        Logger::Info("Jolt A: RegisterDefaultAllocator");
        JPH::RegisterDefaultAllocator();

        // Подключаем trace handler
        JPH::Trace = JoltTraceImpl;

        Logger::Info("Jolt B: Factory");
        JPH::Factory::sInstance = new JPH::Factory();

        Logger::Info("Jolt C: RegisterTypes");
        if (!JPH::Factory::sInstance) {
            Logger::Warn("Factory is null before RegisterTypes!");
            return;
        }
        JPH::RegisterTypes();

        Logger::Info("Jolt D: TempAllocator");
        ph.tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(64 * 1024 * 1024); // 64MB: хватит для 500 NPC

        Logger::Info("Jolt E: JobSystemThreadPool");
        ph.jobSystem = std::make_unique<JPH::JobSystemThreadPool>(
            JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, 2);

        Logger::Info("Jolt F: PhysicsSystem::Init");
        // 500 NPC + 300 кубов + стены + персонаж ≈ 810 тел.
        // Пары контактов и ограничения растут как O(N) — берём с запасом.
        constexpr JPH::uint cMaxBodies        = 8192;
        constexpr JPH::uint cNumBodyMutexes   = 0;
        constexpr JPH::uint cMaxBodyPairs     = 65536;
        constexpr JPH::uint cMaxContactConstr = 32768;

        ph.physicsSystem = std::make_unique<JPH::PhysicsSystem>();
        ph.physicsSystem->Init(
            cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstr,
            s_BPLayerInterface, s_ObjVsBPFilter, s_ObjPairFilter);

        Logger::Info("Jolt G: SetGravity");
        ph.physicsSystem->SetGravity(JPH::Vec3(0.0f, -20.0f, 0.0f));
        ph.bodyInterface = &ph.physicsSystem->GetBodyInterface();

        Logger::Info("Jolt H: ContactListener");
        ph.contactListener = std::make_unique<RKContactListener>();
        ph.physicsSystem->SetContactListener(ph.contactListener.get());

        ph.initialized = true;

        Logger::Info("Jolt Physics initialized.");
        // CharacterVirtual создаётся позже в SceneLoad — после OptimizeBroadPhase
#else
        Logger::Warn("Jolt not compiled in — physics disabled.");
#endif
    }
}
