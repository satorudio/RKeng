#pragma once

// FrameTick — три задачи фрейма, каждая строго отдельная.

namespace RKeng::FrameTick
{
    void ResetTimer();               // сбросить таймер перед стартом цикла
    void PollEvents(bool& running);  // события окна/ввода
    void Update();                   // логика, физика (пока заглушка)
    void Render();                   // отправить кадр в Vulkan
}
