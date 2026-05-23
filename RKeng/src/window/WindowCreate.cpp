#include "WindowCreate.h"
#include "../utils/Logger.h"
#include <GLFW/glfw3.h>
#include <stdexcept>

namespace RKeng::WindowCreate
{
    static void FramebufferResizeCallback(GLFWwindow* window, int /*w*/, int /*h*/)
    {
        auto* win = reinterpret_cast<WindowState*>(glfwGetWindowUserPointer(window));
        win->resized = true;
    }

    static void WindowFocusCallback(GLFWwindow* window, int focused)
    {
        auto* win = reinterpret_cast<WindowState*>(glfwGetWindowUserPointer(window));
        win->focused = (focused == GLFW_TRUE);
    }

    static void WindowIconifyCallback(GLFWwindow* window, int iconified)
    {
        auto* win = reinterpret_cast<WindowState*>(glfwGetWindowUserPointer(window));
        win->iconified = (iconified == GLFW_TRUE);
    }

    void Run(WindowState& win)
    {
        if (!glfwInit())
            throw std::runtime_error("glfwInit failed.");

        // Говорим GLFW: не создавай OpenGL контекст — мы используем Vulkan
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        win.handle = glfwCreateWindow(win.width, win.height, win.title, nullptr, nullptr);
        if (!win.handle)
            throw std::runtime_error("glfwCreateWindow failed.");

        glfwSetWindowUserPointer(win.handle, &win);
        glfwSetFramebufferSizeCallback(win.handle, FramebufferResizeCallback);
        glfwSetWindowFocusCallback(win.handle, WindowFocusCallback);
        glfwSetWindowIconifyCallback(win.handle, WindowIconifyCallback);

        // Захватываем курсор для FPS камеры
        glfwSetInputMode(win.handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        Logger::Info("Window created (" +
            std::to_string(win.width) + "x" + std::to_string(win.height) + ").");
    }
}
