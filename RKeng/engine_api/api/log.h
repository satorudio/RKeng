#pragma once
namespace RKeng {
    struct LogAPI {
        void (*LogInfo )(const char* msg) = nullptr;
        void (*LogWarn )(const char* msg) = nullptr;
        void (*LogError)(const char* msg) = nullptr;
    };
}
