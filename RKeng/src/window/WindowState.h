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
        bool        focused = true;   // false при alt-tab, true при возврате
        bool        iconified = false; // true когда свёрнуто
    };

    WindowState& GetWindowState();
}
