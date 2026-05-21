#include "VulkanSurfaceCreate.h"
#include "../utils/Logger.h"
#include <GLFW/glfw3.h>
#include <stdexcept>

namespace RKeng::VulkanSurfaceCreate
{
    void Run(VulkanState& vk, WindowState& win)
    {
        if (glfwCreateWindowSurface(vk.instance, win.handle, nullptr, &vk.surface) != VK_SUCCESS)
            throw std::runtime_error("Failed to create window surface.");

        // Синхронизируем размер окна в VulkanState
        vk.windowWidth  = static_cast<uint32_t>(win.width);
        vk.windowHeight = static_cast<uint32_t>(win.height);

        Logger::Info("VkSurfaceKHR created.");
    }
}
