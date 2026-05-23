#pragma once
#include "VulkanState.h"

namespace RKeng::VulkanBufferCreate
{
    void CreateBuffer(VulkanState& vk,
                      VkDeviceSize size,
                      VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags props,
                      VkBuffer& buf,
                      VkDeviceMemory& mem);

    void CopyBuffer(VulkanState& vk,
                    VkBuffer src, VkBuffer dst, VkDeviceSize size);

    uint32_t FindMemoryType(VulkanState& vk,
                            uint32_t typeFilter,
                            VkMemoryPropertyFlags props);
    // Инициализирует комнату (вертексы + индексы) и uniform буферы
    void Run(VulkanState& vk);
}
