#pragma once
#include <cstdint>

// FrameTick — три задачи фрейма, каждая строго отдельная.

namespace RKeng::FrameTick
{
    void ResetTimer();
    void PollEvents(bool& running);
    void Update(uint64_t frameNum);   // frameNum нужен для статистики
    void Render();
}
