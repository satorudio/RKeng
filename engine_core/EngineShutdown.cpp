#include "EngineShutdown.h"
#include "EngineInit.h"              // GetSceneLoader()
#include "../vulkan/VulkanContext.h"
#include "../physics/PhysicsState.h"
#include "../scene/SceneState.h"
#include "../utils/Logger.h"

namespace RKeng::EngineShutdown
{
    void Run()
    {
        Logger::Info("=== RKeng Shutdown ===");

        // Сначала даём сцене почиститься (удалить физ. тела, etc.)
        auto& loader = GetSceneLoader();
        if (loader.IsLoaded())
        {
            Logger::Info("Unloading scene plugin: " + loader.GetPath());
            loader.GetPlugin()->OnUnload(GetSceneState(), GetPhysicsState());
            loader.Unload();   // FreeLibrary / dlclose
        }

        VulkanContext::Shutdown();
        Logger::Shutdown();
    }
}
