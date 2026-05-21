#include "VulkanCommandPoolCreate.h"
#include "../utils/Logger.h"
#include <stdexcept>

namespace RKeng::VulkanCommandPoolCreate
{
    void Run(VulkanState& vk)
    {
        // Command pool
        VkCommandPoolCreateInfo poolCI{};
        poolCI.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolCI.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolCI.queueFamilyIndex = vk.graphicsQueueFamily;

        if (vkCreateCommandPool(vk.device, &poolCI, nullptr, &vk.commandPool) != VK_SUCCESS)
            throw std::runtime_error("Failed to create command pool.");

        // Command buffers — по одному на каждый frame in flight
        vk.commandBuffers.resize(VulkanState::MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocCI{};
        allocCI.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocCI.commandPool        = vk.commandPool;
        allocCI.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocCI.commandBufferCount = static_cast<uint32_t>(vk.commandBuffers.size());

        if (vkAllocateCommandBuffers(vk.device, &allocCI, vk.commandBuffers.data()) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate command buffers.");

        Logger::Info("Command pool + buffers created.");
    }
}
