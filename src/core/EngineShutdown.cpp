#include "EngineShutdown.h"
#include "../vulkan/VulkanContext.h"
#include "ScenePluginLoader.h"
#include "SceneState.h"
#include "../physics/PhysicsState.h"
#include "../utils/Logger.h"

namespace RKeng
{
    // Объявлен в EngineInit.cpp
    ScenePluginLoader& GetSceneLoader();
}

namespace RKeng::EngineShutdown
{
    void Run()
    {
        Logger::Info("=== RKeng Shutdown ===");

        // Выгружаем DLL-сцену до того как рушим Vulkan и физику
        auto& loader = GetSceneLoader();
        if (loader.IsLoaded())
        {
            Logger::Info("Unloading DLL scene: " + loader.GetPath());
            loader.GetPlugin()->OnUnload(GetSceneState(), GetPhysicsState());
            loader.Unload();
        }

        VulkanContext::Shutdown();
        Logger::Shutdown();
    }
}
