#include "EngineInit.h"
#include "EngineAPI_Impl.h"          // реализация заполнения EngineAPI
#include "../window/WindowCreate.h"
#include "../window/WindowState.h"
#include "../vulkan/VulkanContext.h"
#include "../physics/PhysicsInit.h"
#include "../physics/PhysicsState.h"
#include "../scene/SceneState.h"
#include "../scene/ScenePluginLoader.h"
#include "../utils/Logger.h"
#include <cstdlib>

// Имя DLL сцены по умолчанию.
// Сменить без рекомпиляции: RK_SCENE_DLL=MyGame.dll ./RKeng
// Сменить при сборке:       cmake -DRK_DEFAULT_SCENE_DLL=MyGame.dll ..
#ifndef RK_DEFAULT_SCENE_DLL
#  ifdef _WIN32
#    define RK_DEFAULT_SCENE_DLL "VoxelCarWorld.dll"
#  else
#    define RK_DEFAULT_SCENE_DLL "./VoxelCarWorld.so"
#  endif
#endif

namespace RKeng
{
    // Живёт здесь — движок владеет лоадером на всё время жизни
    static ScenePluginLoader s_SceneLoader;

    ScenePluginLoader& GetSceneLoader() { return s_SceneLoader; }
}

namespace RKeng::EngineInit
{
    void Run(bool& outRunning)
    {
        Logger::Init();
        Logger::Info("=== RKeng Init ===");

        auto& win  = GetWindowState();
        win.width  = 1920;
        win.height = 1080;
        WindowCreate::Run(win);
        Logger::Info(">>> Step 1: Window OK");

        Logger::Info(">>> Step 2: PhysicsInit...");
        PhysicsInit::Run(GetPhysicsState());
        Logger::Info(">>> Step 2: Physics OK");

        Logger::Info(">>> Step 3: VulkanContext::Init...");
        VulkanContext::Init();
        Logger::Info(">>> Step 3: Vulkan OK");

        // Определяем DLL сцены: env → дефолт
        const char* envDll = std::getenv("RK_SCENE_DLL");
        const std::string dllName = envDll ? envDll : RK_DEFAULT_SCENE_DLL;

        Logger::Info(">>> Step 4: Loading scene plugin '" + dllName + "'...");
        s_SceneLoader.Load(dllName);

        // Заполняем EngineAPI — то что движок предоставляет сцене
        EngineAPI api = EngineAPI_Impl::Build();

        s_SceneLoader.GetPlugin()->OnLoad(GetSceneState(), GetPhysicsState(), api);
        Logger::Info(">>> Step 4: Scene '" +
                     std::string(s_SceneLoader.GetPlugin()->GetName()) + "' loaded OK");

        outRunning = true;
        Logger::Info("Engine ready.");
    }
}
