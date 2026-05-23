#include "VulkanBufferCreate.h"
#include "Logger.h"
#include <stdexcept>
#include <cstring>

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
}
