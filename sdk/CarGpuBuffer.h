#pragma once
#include "CarState.h"
#include "VulkanState.h"

namespace RKeng::CarGpuBuffer
{
    struct CarBuffers
    {
        VkBuffer       vertexBuffer       = VK_NULL_HANDLE;
        VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
        VkBuffer       indexBuffer        = VK_NULL_HANDLE;
        VkDeviceMemory indexBufferMemory  = VK_NULL_HANDLE;
        uint32_t       indexCount         = 0;
        bool           valid              = false;
    };

    // Загрузить/обновить меш машины если meshDirty
    void UploadIfDirty(VulkanState& vk, CarState& car, CarBuffers& buf);

    // Освободить
    void Destroy(VulkanState& vk, CarBuffers& buf);
}
