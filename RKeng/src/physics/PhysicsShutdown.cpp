#include "PhysicsShutdown.h"
#include "../utils/Logger.h"

// Вот этой херни не хватало для полного счастья:
#ifdef RK_JOLT_ENABLED
#include <Jolt/Core/Factory.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#endif

namespace RKeng::PhysicsShutdown
{
    void Run(PhysicsState& ph)
    {
#ifdef RK_JOLT_ENABLED
        // Сначала уничтожаем character — он держит ref на physicsSystem
        ph.character.reset();

        // Затем contactListener (держит обратные ссылки на bodyInterface)
        ph.contactListener.reset();

        // PhysicsSystem — после character, до jobSystem/tempAllocator
        ph.physicsSystem.reset();

        ph.jobSystem.reset();
        ph.tempAllocator.reset();
        ph.bodyInterface = nullptr;

        // UnregisterTypes перед уничтожением Factory (иначе leak RTTI объектов)
        JPH::UnregisterTypes();

        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;

        ph.initialized = false;
        Logger::Info("Jolt Physics shut down.");
#else
        (void)ph;
#endif
    }
}