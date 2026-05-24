#pragma once
#include "VoxelWall.h"
#include "VulkanState.h"
#include <vector>

// VoxelWallBuffer.h — Vulkan буферы для воксельных стен.
// Каждая стена имеет свой vertex/index буфер.
// При разрушении (meshDirty) буферы пересоздаются.

namespace RKeng::VoxelWallBuffer
{
    struct WallGpuBuffers
    {
        VkBuffer       vertexBuffer       = VK_NULL_HANDLE;
        VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
        VkBuffer       indexBuffer        = VK_NULL_HANDLE;
        VkDeviceMemory indexBufferMemory  = VK_NULL_HANDLE;
        uint32_t       indexCount         = 0;
        bool           valid              = false;
    };

    // Создать/обновить буферы для всех стен с meshDirty == true
    void UploadDirtyWalls(VulkanState& vk,
                          std::vector<VoxelWall>& walls,
                          std::vector<WallGpuBuffers>& gpuBuffers);

    // Освободить все буферы (вызывается при shutdown)
    void DestroyAll(VulkanState& vk,
                    std::vector<WallGpuBuffers>& gpuBuffers);
}
