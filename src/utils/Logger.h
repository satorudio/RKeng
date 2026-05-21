#pragma once
#include <string_view>

namespace RKeng::Logger
{
    void Init();
    void Shutdown();

    void Info  (std::string_view msg);
    void Warn  (std::string_view msg);
    void Error (std::string_view msg);
    void Fatal (std::string_view msg);
    void Debug (std::string_view msg);  // только при RK_DEBUG
    void Trace (std::string_view msg);  // только при RK_DEBUG

    // Печатает значение указателя: "[PTR] name = 0x00007ff..."
    // Используется для диагностики Jolt-синглтонов перед OnLoad().
    void Ptr   (std::string_view name, const void* ptr);
}
