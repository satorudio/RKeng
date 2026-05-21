#pragma once

// VulkanContext — дирижёр Vulkan.
// Сам ничего не инициализирует — делегирует в отдельные файлы.

namespace RKeng::VulkanContext
{
    void Init();
    void DrawFrame();
    void Shutdown();
}
