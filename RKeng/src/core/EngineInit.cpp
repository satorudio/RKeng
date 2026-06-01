#include "EngineInit.h"
#include "../window/WindowCreate.h"
#include "../window/WindowState.h"
#include "../vulkan/VulkanContext.h"
#include "../physics/PhysicsInit.h"
#include "../physics/PhysicsState.h"
#include "../input/InputPoll.h"
#include "SceneState.h"
#include "SceneRegistry.h"
#include "ScenePluginLoader.h"
#include "EngineAPI_Impl.h"
#include "../utils/Logger.h"
#include <cstdlib>

#ifndef RK_DEFAULT_SCENE_DLL
#  define RK_DEFAULT_SCENE_DLL ""
#endif

#ifndef RK_DEFAULT_SCENE
#  define RK_DEFAULT_SCENE "template"
#endif

namespace RKeng
{
    ScenePluginLoader* s_SceneLoader = nullptr;
    ScenePluginLoader& GetSceneLoader() { return *s_SceneLoader; }
}

namespace RKeng::EngineInit
{
    static std::string trimStr(const char* s)
    {
        if (!s) return {};
        std::string r(s);
        while (!r.empty() && (r.back()  == ' ' || r.back()  == '\t' ||
                               r.back()  == '\r'|| r.back()  == '\n')) r.pop_back();
        while (!r.empty() && (r.front() == ' ' || r.front() == '\t')) r.erase(r.begin());
        return r;
    }

    void Run(bool& outRunning)
    {
        s_SceneLoader = new ScenePluginLoader();

        Logger::Init();
        Logger::Info("=== RKeng Init ===");

        // ── Step 1: Window ───────────────────────────────────────────────────
        auto& win  = GetWindowState();
        win.width  = 1920;
        win.height = 1080;
        WindowCreate::Run(win);
        GetSceneState().windowHandle = win.handle;
        InputPoll::Init();  // регистрируем focus callback — до Vulkan init
        Logger::Info(">>> Step 1: Window OK");

        // ── Step 2: Physics ──────────────────────────────────────────────────
        Logger::Info(">>> Step 2: PhysicsInit...");
        PhysicsInit::Run(GetPhysicsState());
        Logger::Info(">>> Step 2: Physics OK");

        // ── Step 3: Vulkan ───────────────────────────────────────────────────
        Logger::Info(">>> Step 3: VulkanContext::Init...");
        VulkanContext::Init();
        Logger::Info(">>> Step 3: Vulkan OK");

        // ── Step 4: Scene ────────────────────────────────────────────────────
        Logger::Info(">>> Step 4: SceneLoad...");

        const std::string activeDll = []() -> std::string {
            std::string e = trimStr(std::getenv("RK_SCENE_DLL"));
            if (!e.empty()) return e;
            return std::string(RK_DEFAULT_SCENE_DLL);
        }();

        if (!activeDll.empty())
        {
            Logger::Info(">>> Step 4: Loading DLL scene '" + activeDll + "'...");
            try
            {
                s_SceneLoader->Load(activeDll);

                EngineAPI api = EngineAPI_Impl::Build();
                s_SceneLoader->GetPlugin()->OnLoad(GetSceneState(), api);
                Logger::Info(std::string(">>> Step 4: DLL scene '") +
                             s_SceneLoader->GetPlugin()->GetName() + "' loaded OK");
            }
            catch (const std::exception& e)
            {
                Logger::Error(std::string("SceneLoad (DLL) failed: ") + e.what());
                Logger::Error("Убедись что " + activeDll + " лежит рядом с RKeng.exe");
            }
        }
        else
        {
            const std::string activeScene = [&]() -> std::string {
                std::string e = trimStr(std::getenv("RK_SCENE"));
                if (!e.empty()) return e;
                return std::string(RK_DEFAULT_SCENE);
            }();
            Logger::Info(">>> Step 4: Loading built-in scene '" + activeScene + "'...");
            try
            {
                SceneRegistry::Load(activeScene, GetSceneState(), GetPhysicsState());
                Logger::Info(">>> Step 4: Built-in scene '" + activeScene + "' loaded OK");
            }
            catch (const std::exception& e)
            {
                Logger::Error(std::string("SceneLoad (Registry) failed: ") + e.what());
            }
        }

        outRunning = true;
        Logger::Info("Engine ready.");
    }
}
