#include "EngineLoop.h"
#include "FrameTick.h"
#include "../vulkan/VulkanContext.h"
#include "../window/WindowState.h"
#include "../utils/Logger.h"
#include <cstdint>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>  // Sleep()
#else
#include <unistd.h>   // usleep()
#endif

namespace RKeng::EngineLoop
{
    void Run(bool& running)
    {
        Logger::Info("Engine loop started.");
        FrameTick::ResetTimer();

        uint64_t frameNum = 0;
        while (running)
        {
            Logger::SetFrame(frameNum);

            FrameTick::PollEvents(running);

            auto& win = GetWindowState();

            // Окно свёрнуто или потеряло фокус — не рендерим совсем,
            // ждём возврата. Это предотвращает таймаут AMD драйвера
            // на vkAcquireNextImageKHR при alt-tab.
            if (win.iconified || !win.focused)
            {
#ifdef _WIN32
                Sleep(50); // 50 мс — 20 "обновлений" в секунду пока свёрнуто
#else
                usleep(50000);
#endif
                // ResetTimer чтобы после возврата не было огромного dt
                FrameTick::ResetTimer();
                continue;
            }

            Logger::Trace("---- Frame " + std::to_string(frameNum) + " ----");
            FrameTick::Update(frameNum);
            FrameTick::Render();

            frameNum++;
        }
    }
}
