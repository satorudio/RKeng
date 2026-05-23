#pragma once
#include <string_view>

namespace RKeng::Logger
{
    void Init();
    void Shutdown();

    void Info  (std::string_view msg);
    void Warn  (std::string_view msg);
    void Error (std::string_view msg);
    void Fatal (std::string_view msg);  // красный + жирный, flush stderr
    void Debug (std::string_view msg);  // серый, только в RK_DEBUG
    void Trace (std::string_view msg);  // тёмно-серый, очень подробно
}
