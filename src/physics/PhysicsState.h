#pragma once
#include <memory>

#ifdef RK_JOLT_ENABLED
#include <Jolt/Jolt.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include "RKContactListener.h"
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>

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
        std::unique_ptr<JPH::CharacterVirtual>     character;   // создаётся сценой после OptimizeBroadPhase
        JPH::CharacterVirtualSettings              characterSettings;
        JPH::BodyInterface*                        bodyInterface = nullptr;
        std::unique_ptr<RKContactListener>         contactListener;
#endif
        bool  initialized   = false;

        // Фиксированный шаг физики
        float fixedTimestep = 1.0f / 60.0f;
        float accumulator   = 0.0f;
        int   collisionSteps = 1;
    };

    PhysicsState& GetPhysicsState();
}
