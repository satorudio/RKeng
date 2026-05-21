#include "FrameTick.h"
#include "../vulkan/VulkanContext.h"
#include "../window/WindowState.h"
#include "../input/InputPoll.h"
#include "../physics/PhysicsTick.h"
#include "SceneState.h"
#include "ScenePluginLoader.h"
#include "../physics/PhysicsState.h"
#include "../utils/Logger.h"
#include <GLFW/glfw3.h>
#include <chrono>

namespace RKeng
{
    ScenePluginLoader& GetSceneLoader();
}

namespace RKeng::FrameTick
{
    static auto s_LastTime = std::chrono::high_resolution_clock::now();

    void ResetTimer()
    {
        s_LastTime = std::chrono::high_resolution_clock::now();
    }

    void PollEvents(bool& running)
    {
        auto* win = GetWindowState().handle;
        if (!win || glfwWindowShouldClose(win))
            running = false;

        auto& scene = GetSceneState();
        InputPoll::Run(scene.input, running);
    }

    void Update()
    {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - s_LastTime).count();
        s_LastTime = now;
        dt = (dt > 0.1f) ? 0.1f : dt;

        auto& scene   = GetSceneState();
        auto& physics = GetPhysicsState();

        scene.deltaTime  = dt;
        scene.totalTime += dt;

        PhysicsTick::Run(physics, dt);

        // Тик DLL-сцены — она делает PlayerMove, CarInputPoll, CarTick и т.д.
        auto& loader = GetSceneLoader();
        if (loader.IsLoaded())
            loader.GetPlugin()->OnTick(scene, physics, dt);
    }

    void Render()
    {
        VulkanContext::DrawFrame();
    }
}
