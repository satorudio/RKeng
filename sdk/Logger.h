#pragma once
#include <string_view>
#include <cstdint>

namespace RKeng::Logger
{
    void Init();
    void Shutdown();

    // Вызывается из EngineLoop каждый кадр.
    // После VERBOSE_FRAMES кадров Info/Debug/Trace замолкают.
    void SetFrame(uint64_t frame);
    uint64_t GetFrame();

    // VERBOSE_FRAMES — первые N кадров логируем всё подряд.
    static constexpr uint64_t VERBOSE_FRAMES = 60;

    void Info  (std::string_view msg);  // молчит после VERBOSE_FRAMES
    void Warn  (std::string_view msg);  // всегда
    void Error (std::string_view msg);  // всегда
    void Fatal (std::string_view msg);  // всегда
    void Debug (std::string_view msg);  // молчит после VERBOSE_FRAMES, только RK_DEBUG
    void Trace (std::string_view msg);  // молчит после VERBOSE_FRAMES, только RK_DEBUG

    // Всегда печатает (статистика, важные события).
    void Always(std::string_view msg);

    void Ptr   (std::string_view name, const void* ptr);
}
