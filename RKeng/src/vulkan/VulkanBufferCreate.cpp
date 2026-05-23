#include "VulkanBufferCreate.h"
#include "../utils/Logger.h"
#include <stdexcept>
#include <cstring>
#include <array>
#include <vector>

namespace RKeng::VulkanBufferCreate
{
    uint32_t FindMemoryType(VulkanState& vk, uint32_t typeFilter, VkMemoryPropertyFlags props)
    {
        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(vk.physicalDevice, &memProps);
        for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
            if ((typeFilter & (1 << i)) &&
                (memProps.memoryTypes[i].propertyFlags & props) == props)
                return i;
        throw std::runtime_error("Failed to find suitable memory type.");
    }

    void CreateBuffer(VulkanState& vk, VkDeviceSize size,
                      VkBufferUsageFlags usage, VkMemoryPropertyFlags props,
                      VkBuffer& buf, VkDeviceMemory& mem)
    {
        VkBufferCreateInfo bi{};
        bi.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bi.size        = size;
        bi.usage       = usage;
        bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateBuffer(vk.device, &bi, nullptr, &buf) != VK_SUCCESS)
            throw std::runtime_error("Failed to create buffer.");

        VkMemoryRequirements req;
        vkGetBufferMemoryRequirements(vk.device, buf, &req);

        VkMemoryAllocateInfo ai{};
        ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        ai.allocationSize  = req.size;
        ai.memoryTypeIndex = FindMemoryType(vk, req.memoryTypeBits, props);
        if (vkAllocateMemory(vk.device, &ai, nullptr, &mem) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate buffer memory.");
        vkBindBufferMemory(vk.device, buf, mem, 0);
    }

    void CopyBuffer(VulkanState& vk, VkBuffer src, VkBuffer dst, VkDeviceSize size)
    {
        VkCommandBufferAllocateInfo ai{};
        ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandPool        = vk.commandPool;
        ai.commandBufferCount = 1;

        VkCommandBuffer cmd;
        vkAllocateCommandBuffers(vk.device, &ai, &cmd);

        VkCommandBufferBeginInfo bi{};
        bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &bi);

        VkBufferCopy region{ 0, 0, size };
        vkCmdCopyBuffer(cmd, src, dst, 1, &region);
        vkEndCommandBuffer(cmd);

        VkSubmitInfo si{};
        si.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        si.commandBufferCount = 1;
        si.pCommandBuffers    = &cmd;
        vkQueueSubmit(vk.graphicsQueue, 1, &si, VK_NULL_HANDLE);
        vkQueueWaitIdle(vk.graphicsQueue);
        vkFreeCommandBuffers(vk.device, vk.commandPool, 1, &cmd);
    }

    struct Vertex {
        float pos[3];
        float normal[3];
        float color[3];
    };

    static void AddQuad(std::vector<Vertex>& verts, std::vector<uint32_t>& idx,
                        Vertex v0, Vertex v1, Vertex v2, Vertex v3)
    {
        uint32_t base = static_cast<uint32_t>(verts.size());
        verts.push_back(v0); verts.push_back(v1);
        verts.push_back(v2); verts.push_back(v3);
        idx.push_back(base+0); idx.push_back(base+1); idx.push_back(base+2);
        idx.push_back(base+0); idx.push_back(base+2); idx.push_back(base+3);
    }

    void Run(VulkanState& vk)
    {
        // ── Открытый мир: тайловый пол 400x400 м ───────────────────────────
        std::vector<Vertex> verts;
        std::vector<uint32_t> idx;

        constexpr float WORLD = 200.0f;
        constexpr float TILE  = 10.0f;
        const int TILES = (int)(WORLD * 2.0f / TILE); // 40 тайлов

        for (int tz = 0; tz < TILES; tz++)
        for (int tx = 0; tx < TILES; tx++)
        {
            float x0 = -WORLD + tx * TILE;
            float x1 = x0 + TILE;
            float z0 = -WORLD + tz * TILE;
            float z1 = z0 + TILE;
            bool  chk = (tx + tz) % 2 == 0;
            float c   = chk ? 0.38f : 0.30f;
            AddQuad(verts, idx,
                {{x0, 0.0f, z0}, {0,1,0}, {c, c, c*0.95f}},
                {{x1, 0.0f, z0}, {0,1,0}, {c, c, c*0.95f}},
                {{x1, 0.0f, z1}, {0,1,0}, {c, c, c*0.95f}},
                {{x0, 0.0f, z1}, {0,1,0}, {c, c, c*0.95f}});
        }

        vk.indexCount = static_cast<uint32_t>(idx.size());

        // ── Vertex buffer ────────────────────────────────────────────────────
        VkDeviceSize vSize = sizeof(Vertex) * verts.size();
        VkBuffer stagingV; VkDeviceMemory stagingVMem;
        CreateBuffer(vk, vSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingV, stagingVMem);
        void* vData; vkMapMemory(vk.device, stagingVMem, 0, vSize, 0, &vData);
        memcpy(vData, verts.data(), vSize); vkUnmapMemory(vk.device, stagingVMem);

        CreateBuffer(vk, vSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            vk.vertexBuffer, vk.vertexBufferMemory);
        CopyBuffer(vk, stagingV, vk.vertexBuffer, vSize);
        vkDestroyBuffer(vk.device, stagingV, nullptr);
        vkFreeMemory(vk.device, stagingVMem, nullptr);

        // ── Index buffer ─────────────────────────────────────────────────────
        VkDeviceSize iSize = sizeof(uint32_t) * idx.size();
        VkBuffer stagingI; VkDeviceMemory stagingIMem;
        CreateBuffer(vk, iSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingI, stagingIMem);
        void* iData; vkMapMemory(vk.device, stagingIMem, 0, iSize, 0, &iData);
        memcpy(iData, idx.data(), iSize); vkUnmapMemory(vk.device, stagingIMem);

        CreateBuffer(vk, iSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            vk.indexBuffer, vk.indexBufferMemory);
        CopyBuffer(vk, stagingI, vk.indexBuffer, iSize);
        vkDestroyBuffer(vk.device, stagingI, nullptr);
        vkFreeMemory(vk.device, stagingIMem, nullptr);

        // ── Uniform buffers (per frame) ───────────────────────────────────────
        VkDeviceSize uSize = sizeof(float) * 16 * 3; // model+view+proj
        vk.uniformBuffers.resize(VulkanState::MAX_FRAMES_IN_FLIGHT);
        vk.uniformBuffersMemory.resize(VulkanState::MAX_FRAMES_IN_FLIGHT);
        vk.uniformBuffersMapped.resize(VulkanState::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < VulkanState::MAX_FRAMES_IN_FLIGHT; i++) {
            CreateBuffer(vk, uSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                vk.uniformBuffers[i], vk.uniformBuffersMemory[i]);
            vkMapMemory(vk.device, vk.uniformBuffersMemory[i], 0, uSize, 0,
                        &vk.uniformBuffersMapped[i]);
        }

        Logger::Info("Room mesh + uniform buffers created.");
    }
}
