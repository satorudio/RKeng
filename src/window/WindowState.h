#pragma once

struct GLFWwindow;

namespace RKeng
{
    struct WindowState
    {
        GLFWwindow* handle  = nullptr;
        int         width   = 1920;
        int         height  = 1080;
        const char* title   = "RKeng";
        bool        resized = false;
    };

    WindowState& GetWindowState();
}
