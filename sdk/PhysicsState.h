#pragma once
#include <memory>

#ifdef RK_JOLT_ENABLED
#include <Jolt/Jolt.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include "RKContactListener.h"

// CapsuleShape.h, RotatedTranslatedShape.h, CharacterVirtual.h включаются
// только там где нужны (SceneLoad.cpp) — не здесь.
// JPH_IMPLEMENT_RTTI_VIRTUAL из этих хедеров создаёт глобальные объекты,
// что безопасно в exe/движке, но недопустимо в SDK-заголовке (DLL-плагин).
namespace JPH { class CharacterVirtual; }

namespace RKeng
{
    namespace PhysLayers {
        static constexpr JPH::ObjectLayer STATIC  = 0;
        static constexpr JPH::ObjectLayer DYNAMIC = 1;
        static constexpr JPH::uint        COUNT   = 2;
    }

    namespace BPLayers {
        static constexpr JPH::BroadPhaseLayer NON_MOVING { 0 };
        static constexpr JPH::BroadPhaseLayer MOVING     { 1 };
        static constexpr JPH::uint            COUNT      = 2;
    }
}
#endif // RK_JOLT_ENABLED

namespace RKeng
{
    struct PhysicsState
    {
#ifdef RK_JOLT_ENABLED
        std::unique_ptr<JPH::TempAllocatorImpl>    tempAllocator;
        std::unique_ptr<JPH::JobSystemThreadPool>  jobSystem;
        std::unique_ptr<JPH::PhysicsSystem>        physicsSystem;
        // CharacterVirtual через unique_ptr — forward declaration достаточно.
        // characterSettings перенесён в SceneLoad.cpp (движок) — в SDK не нужен.
        std::unique_ptr<JPH::CharacterVirtual>     character;
        JPH::BodyInterface*                        bodyInterface = nullptr;
        std::unique_ptr<RKContactListener>         contactListener;
#endif
        bool  initialized   = false;

        float fixedTimestep  = 1.0f / 60.0f;
        float accumulator    = 0.0f;
        int   collisionSteps = 1;
    };

    PhysicsState& GetPhysicsState();
}
