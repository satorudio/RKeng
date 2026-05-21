#include "EngineLoop.h"
#include "FrameTick.h"
#include "../vulkan/VulkanContext.h"
#include "../utils/Logger.h"
#include <cstdint>
#include <string>

namespace RKeng::EngineLoop
{
    void Run(bool& running)
    {
        Logger::Info("Engine loop started.");
        FrameTick::ResetTimer();  // сбрасываем таймер прямо перед стартом цикла

        uint64_t frameNum = 0;
        while (running)
        {
            Logger::Trace("---- Frame " + std::to_string(frameNum) + " ----");
            Logger::Trace("  PollEvents...");
            FrameTick::PollEvents(running);
            Logger::Trace("  Update...");
            FrameTick::Update();
            Logger::Trace("  Render...");
            FrameTick::Render();
            Logger::Trace("  Frame " + std::to_string(frameNum) + " done");
            frameNum++;
        }
    }
}
