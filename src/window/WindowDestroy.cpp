#include "WindowDestroy.h"
#include "../utils/Logger.h"
#include <GLFW/glfw3.h>

namespace RKeng::WindowDestroy
{
    void Run(WindowState& win)
    {
        if (win.handle) glfwDestroyWindow(win.handle);
        glfwTerminate();
        win.handle = nullptr;
        Logger::Info("Window destroyed.");
    }
}
