#include "FrameTick.h"
#include "../vulkan/VulkanContext.h"
#include "../vulkan/VulkanState.h"
#include "../window/WindowState.h"
#include "../input/InputPoll.h"
#include "../physics/PhysicsTick.h"
#include "SceneState.h"
#include "ScenePluginLoader.h"
#include "../physics/PhysicsState.h"
#include "../utils/Logger.h"
#include "../math/Camera.h"
#include "../math/Frustum.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <string>
#include <deque>
#include <numeric>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>          // GetProcessMemoryInfo
#pragma comment(lib, "psapi.lib")
#endif

// Vulkan memory budget (VK_EXT_memory_budget)
#include <vulkan/vulkan.h>

namespace RKeng
{
    ScenePluginLoader& GetSceneLoader();
    VulkanState& GetVulkanState();
}

namespace RKeng::FrameTick
{
    using Clock = std::chrono::high_resolution_clock;
    using Tp    = Clock::time_point;

    static Tp       s_LastTime;
    static Tp       s_StatTime;        // начало текущего stat-окна
    static uint64_t s_StatFrames = 0;  // кадры в текущем окне
    static double   s_StatAccMs  = 0;  // накопленное время кадра (мс) в окне

    static constexpr uint64_t STAT_INTERVAL = 60; // каждые N кадров

    // ── Платформенная статистика ───────────────────────────────────────────
    struct PerfStats { float ramMB; float cpuPct; float gpuMB; };

    static PerfStats QueryPerfStats()
    {
        PerfStats s{};

#ifdef _WIN32
        // ── RAM (рабочий набор процесса) ──────────────────────────────────
        PROCESS_MEMORY_COUNTERS pmc{};
        pmc.cb = sizeof(pmc);
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
            s.ramMB = static_cast<float>(pmc.WorkingSetSize) / (1024.f * 1024.f);

        // ── CPU (разница KERNEL+USER времени за ~один кадр) ───────────────
        // Считаем один раз при вызове — delta к предыдущему вызову.
        static ULARGE_INTEGER s_prevKernel{}, s_prevUser{}, s_prevWall{};
        FILETIME ftCreate, ftExit, ftKernel, ftUser, ftNow;
        GetProcessTimes(GetCurrentProcess(), &ftCreate, &ftExit, &ftKernel, &ftUser);
        GetSystemTimeAsFileTime(&ftNow);

        ULARGE_INTEGER curK, curU, curW;
        curK.LowPart = ftKernel.dwLowDateTime; curK.HighPart = ftKernel.dwHighDateTime;
        curU.LowPart = ftUser.dwLowDateTime;   curU.HighPart = ftUser.dwHighDateTime;
        curW.LowPart = ftNow.dwLowDateTime;    curW.HighPart = ftNow.dwHighDateTime;

        if (s_prevWall.QuadPart != 0)
        {
            auto dCpu  = (curK.QuadPart - s_prevKernel.QuadPart)
                       + (curU.QuadPart - s_prevUser.QuadPart);
            auto dWall =  curW.QuadPart - s_prevWall.QuadPart;
            if (dWall > 0)
            {
                SYSTEM_INFO si{};
                GetSystemInfo(&si);
                s.cpuPct = static_cast<float>(dCpu * 100.0 / dWall / si.dwNumberOfProcessors);
            }
        }
        s_prevKernel = curK; s_prevUser = curU; s_prevWall = curW;
#endif

        // ── GPU VRAM (VK_EXT_memory_budget) ───────────────────────────────
        auto& vk = GetVulkanState();
        if (vk.physicalDevice != VK_NULL_HANDLE)
        {
            VkPhysicalDeviceMemoryBudgetPropertiesEXT budget{};
            budget.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;

            VkPhysicalDeviceMemoryProperties2 props2{};
            props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
            props2.pNext = &budget;

            vkGetPhysicalDeviceMemoryProperties2(vk.physicalDevice, &props2);

            VkDeviceSize used = 0;
            for (uint32_t i = 0; i < props2.memoryProperties.memoryHeapCount; ++i)
                if (props2.memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                    used += budget.heapUsage[i];

            s.gpuMB = static_cast<float>(used) / (1024.f * 1024.f);
        }

        return s;
    }

    // ── Форматирование строки статистики ──────────────────────────────────
    static std::string FmtStats(uint64_t frameNum, double avgMs, double fps,
                                 const PerfStats& p)
    {
        std::ostringstream o;
        o << std::fixed << std::setprecision(2);
        o << "frame=" << frameNum
          << "  fps=" << fps
          << "  avg=" << avgMs << "ms"
          << "  ram=" << p.ramMB << "MB"
          << "  cpu=" << p.cpuPct << "%"
          << "  gpu=" << p.gpuMB << "MB";
        return o.str();
    }

    // ── Публичный API ──────────────────────────────────────────────────────

    void ResetTimer()
    {
        s_LastTime  = Clock::now();
        s_StatTime  = s_LastTime;
        s_StatFrames = 0;
        s_StatAccMs  = 0;
    }

    void PollEvents(bool& running)
    {
        auto* win = GetWindowState().handle;
        if (!win || glfwWindowShouldClose(win))
            running = false;

        auto& scene = GetSceneState();
        InputPoll::Run(scene.input, running);
    }

    void Update(uint64_t frameNum)
    {
        auto now = Clock::now();
        double dtMs = std::chrono::duration<double, std::milli>(now - s_LastTime).count();
        float  dt   = static_cast<float>(dtMs / 1000.0);
        s_LastTime  = now;
        if (dt > 0.1f) dt = 0.1f;

        auto& scene   = GetSceneState();
        auto& physics = GetPhysicsState();

        scene.deltaTime  = dt;
        scene.totalTime += dt;

        PhysicsTick::Run(physics, dt);

        // ── Обновляем фрустум ПЕРЕД OnTick чтобы плагин видел фрустум текущего кадра ──
        {
            auto& vk     = GetVulkanState();
            auto& player = scene.player;
            auto& input  = scene.input;

            float yaw   = glm::radians(input.yaw);
            float pitch = glm::radians(input.pitch);

            glm::vec3 pos = player.worldPos.ToLocal(scene.originShift);
            if (!scene.thirdPersonCamera)
                pos.y += player.currentHeight * 0.85f;

            glm::vec3 front;
            front.x = glm::sin(yaw) * glm::cos(pitch);
            front.y = glm::sin(pitch);
            front.z = -glm::cos(yaw) * glm::cos(pitch);
            front   = glm::normalize(front);

            float nearPlane = player.isCrouching ? 0.01f : 0.05f;
            float aspect    = (vk.scExtent.height > 0)
                ? static_cast<float>(vk.scExtent.width) / static_cast<float>(vk.scExtent.height)
                : 16.f / 9.f;

            Camera cam;
            cam.position  = pos;
            cam.target    = pos + front;
            cam.up        = glm::vec3(0, 1, 0);
            cam.fovDeg    = 90.0f;
            cam.aspect    = aspect;
            cam.nearPlane = nearPlane;
            cam.farPlane  = 500.0f;

            Frustum fr       = FrustumOps::BuildFromCamera(cam);
            scene.frustumPlanes = fr.planes;
            scene.frustumReady  = true;
        }

        auto& loader = GetSceneLoader();
        if (loader.IsLoaded())
            loader.GetPlugin()->OnTick(scene, dt);

        // ── Статистика каждые STAT_INTERVAL кадров ────────────────────────
        s_StatFrames++;
        s_StatAccMs += dtMs;

        if (s_StatFrames >= STAT_INTERVAL)
        {
            double avgMs = s_StatAccMs / s_StatFrames;
            double fps   = (avgMs > 0) ? 1000.0 / avgMs : 0.0;
            auto   perf  = QueryPerfStats();
            Logger::Always(FmtStats(frameNum, avgMs, fps, perf));

            s_StatFrames = 0;
            s_StatAccMs  = 0;
        }
    }

    void Render()
    {
        VulkanContext::DrawFrame();
    }
}
