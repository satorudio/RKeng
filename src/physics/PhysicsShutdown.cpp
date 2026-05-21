#include "PhysicsShutdown.h"
#include "../utils/Logger.h"

// Вот этой херни не хватало для полного счастья:
#ifdef RK_JOLT_ENABLED
#include <Jolt/Core/Factory.h>
#endif

namespace RKeng::PhysicsShutdown
{
    void Run(PhysicsState& ph)
    {
#ifdef RK_JOLT_ENABLED
        ph.character.reset();
        ph.physicsSystem.reset();
        ph.jobSystem.reset();
        ph.tempAllocator.reset();

        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;

        ph.initialized = false;
        Logger::Info("Jolt Physics shut down.");
#else
        (void)ph;
#endif
    }
}