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
        FrameTick::ResetTimer();

        uint64_t frameNum = 0;
        while (running)
        {
            Logger::SetFrame(frameNum);

            Logger::Trace("---- Frame " + std::to_string(frameNum) + " ----");
            FrameTick::PollEvents(running);
            FrameTick::Update(frameNum);
            FrameTick::Render();

            frameNum++;
        }
    }
}
