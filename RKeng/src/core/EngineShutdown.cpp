#include "EngineShutdown.h"
#include "../vulkan/VulkanContext.h"
#include "ScenePluginLoader.h"
#include "SceneState.h"
#include "../physics/PhysicsState.h"
#include "../physics/PhysicsShutdown.h"
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

        // ── 1. Выгружаем DLL-сцену до того как рушим Vulkan и физику ─────────
        try
        {
            auto& loader = GetSceneLoader();
            if (loader.IsLoaded())
            {
                Logger::Info("Unloading DLL scene: " + loader.GetPath());
                loader.GetPlugin()->OnUnload(GetSceneState(), GetPhysicsState());
                loader.Unload();
                Logger::Info("DLL scene unloaded OK.");
            }
        }
        catch (const std::exception& e)
        {
            Logger::Error(std::string("[Shutdown] Scene OnUnload threw: ") + e.what());
        }
        catch (...)
        {
            Logger::Error("[Shutdown] Scene OnUnload threw unknown exception");
        }

        // ── 2. Vulkan ─────────────────────────────────────────────────────────
        try
        {
            VulkanContext::Shutdown();
            Logger::Info("[Shutdown] Vulkan OK.");
        }
        catch (const std::exception& e)
        {
            Logger::Error(std::string("[Shutdown] Vulkan threw: ") + e.what());
        }
        catch (...)
        {
            Logger::Error("[Shutdown] Vulkan threw unknown exception");
        }

        // ── 3. Physics ────────────────────────────────────────────────────────
        try
        {
            PhysicsShutdown::Run(GetPhysicsState());
            Logger::Info("[Shutdown] Physics OK.");
        }
        catch (const std::exception& e)
        {
            Logger::Error(std::string("[Shutdown] Physics threw: ") + e.what());
        }
        catch (...)
        {
            Logger::Error("[Shutdown] Physics threw unknown exception");
        }

        Logger::Shutdown();
    }
}

