#pragma once
#include "RKExport.h"
#include <string_view>
#include <cstdint>

namespace RKeng::Logger
{
    RK_API void Init();
    RK_API void Shutdown();

    RK_API void SetFrame(uint64_t frame);
    RK_API uint64_t GetFrame();

    static constexpr uint64_t VERBOSE_FRAMES = 60;

    RK_API void Info  (std::string_view msg);
    RK_API void Warn  (std::string_view msg);
    RK_API void Error (std::string_view msg);
    RK_API void Fatal (std::string_view msg);
    RK_API void Debug (std::string_view msg);
    RK_API void Trace (std::string_view msg);
    RK_API void Always(std::string_view msg);
    RK_API void Ptr   (std::string_view name, const void* ptr);
}
