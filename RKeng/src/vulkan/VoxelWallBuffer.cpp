#include "VoxelWallBuffer.h"
#include "../vulkan/VulkanBufferCreate.h"
#include "../utils/Logger.h"
#include <cstring>

namespace RKeng::VoxelWallBuffer
{
    static void DestroyWallBuffers(VulkanState& vk, WallGpuBuffers& gb)
    {
        if (gb.vertexBuffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(vk.device, gb.vertexBuffer, nullptr);
            vkFreeMemory(vk.device, gb.vertexBufferMemory, nullptr);
            gb.vertexBuffer = VK_NULL_HANDLE;
        }
        if (gb.indexBuffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(vk.device, gb.indexBuffer, nullptr);
            vkFreeMemory(vk.device, gb.indexBufferMemory, nullptr);
            gb.indexBuffer = VK_NULL_HANDLE;
        }
        gb.valid = false;
        gb.indexCount = 0;
    }

    static void UploadWall(VulkanState& vk, const VoxelWall& wall, WallGpuBuffers& gb)
    {
        // Освобождаем старые буферы
        DestroyWallBuffers(vk, gb);

        // ВАЖНО: Vulkan не принимает буфер нулевого размера — всегда проверяем!
        if (wall.vertices.empty() || wall.indices.empty())
        {
            gb.valid      = false;
            gb.indexCount = 0;
            return;
        }

        // ---- Vertex buffer ----
        VkDeviceSize vbSize = sizeof(VoxelVertex) * wall.vertices.size();

        VkBuffer stagingV; VkDeviceMemory stagingVMem;
        VulkanBufferCreate::CreateBuffer(vk, vbSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingV, stagingVMem);

        void* dataV;
        vkMapMemory(vk.device, stagingVMem, 0, vbSize, 0, &dataV);
        memcpy(dataV, wall.vertices.data(), vbSize);
        vkUnmapMemory(vk.device, stagingVMem);

        VulkanBufferCreate::CreateBuffer(vk, vbSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            gb.vertexBuffer, gb.vertexBufferMemory);

        VulkanBufferCreate::CopyBuffer(vk, stagingV, gb.vertexBuffer, vbSize);
        vkDestroyBuffer(vk.device, stagingV, nullptr);
        vkFreeMemory(vk.device, stagingVMem, nullptr);

        // ---- Index buffer ----
        VkDeviceSize ibSize = sizeof(uint32_t) * wall.indices.size();

        VkBuffer stagingI; VkDeviceMemory stagingIMem;
        VulkanBufferCreate::CreateBuffer(vk, ibSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingI, stagingIMem);

        void* dataI;
        vkMapMemory(vk.device, stagingIMem, 0, ibSize, 0, &dataI);
        memcpy(dataI, wall.indices.data(), ibSize);
        vkUnmapMemory(vk.device, stagingIMem);

        VulkanBufferCreate::CreateBuffer(vk, ibSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            gb.indexBuffer, gb.indexBufferMemory);

        VulkanBufferCreate::CopyBuffer(vk, stagingI, gb.indexBuffer, ibSize);
        vkDestroyBuffer(vk.device, stagingI, nullptr);
        vkFreeMemory(vk.device, stagingIMem, nullptr);

        gb.indexCount = static_cast<uint32_t>(wall.indices.size());
        gb.valid      = true;
    }

    // ----------------------------------------------------------------
    // Public API
    // ----------------------------------------------------------------
    void UploadDirtyWalls(VulkanState& vk,
                          std::vector<VoxelWall>& walls,
                          std::vector<WallGpuBuffers>& gpuBuffers)
    {
        // Убедимся что gpuBuffers нужного размера
        if (gpuBuffers.size() != walls.size())
            gpuBuffers.resize(walls.size());

        for (size_t i = 0; i < walls.size(); i++)
        {
            if (!walls[i].meshDirty) continue;

            // GPU-idle гарантирован вызывающей стороной (после vkWaitForFences)
            UploadWall(vk, walls[i], gpuBuffers[i]);
            walls[i].meshDirty = false;
        }
    }

    void DestroyAll(VulkanState& vk, std::vector<WallGpuBuffers>& gpuBuffers)
    {
        for (auto& gb : gpuBuffers)
            DestroyWallBuffers(vk, gb);
        gpuBuffers.clear();
    }
}
