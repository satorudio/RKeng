#include "VulkanDepthCreate.h"
#include "../utils/Logger.h"
#include <stdexcept>
#include <array>

namespace RKeng::VulkanDepthCreate
{
    static uint32_t FindMemoryType(VkPhysicalDevice physDev, uint32_t typeFilter, VkMemoryPropertyFlags props)
    {
        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(physDev, &memProps);
        for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
            if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & props) == props)
                return i;
        throw std::runtime_error("Failed to find suitable memory type for depth buffer.");
    }

    static VkFormat FindDepthFormat(VkPhysicalDevice physDev)
    {
        std::array<VkFormat, 3> candidates = {
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT
        };
        for (VkFormat fmt : candidates)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physDev, fmt, &props);
            if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
                return fmt;
        }
        throw std::runtime_error("Failed to find supported depth format.");
    }

    void Run(VulkanState& vk)
    {
        vk.depthFormat = FindDepthFormat(vk.physicalDevice);

        // Создаём depth image
        VkImageCreateInfo imgCI{};
        imgCI.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imgCI.imageType     = VK_IMAGE_TYPE_2D;
        imgCI.format        = vk.depthFormat;
        imgCI.extent        = { vk.scExtent.width, vk.scExtent.height, 1 };
        imgCI.mipLevels     = 1;
        imgCI.arrayLayers   = 1;
        imgCI.samples       = VK_SAMPLE_COUNT_1_BIT;
        imgCI.tiling        = VK_IMAGE_TILING_OPTIMAL;
        imgCI.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imgCI.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        imgCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (vkCreateImage(vk.device, &imgCI, nullptr, &vk.depthImage) != VK_SUCCESS)
            throw std::runtime_error("Failed to create depth image.");

        VkMemoryRequirements memReq;
        vkGetImageMemoryRequirements(vk.device, vk.depthImage, &memReq);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize  = memReq.size;
        allocInfo.memoryTypeIndex = FindMemoryType(vk.physicalDevice, memReq.memoryTypeBits,
                                                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(vk.device, &allocInfo, nullptr, &vk.depthImageMemory) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate depth image memory.");

        vkBindImageMemory(vk.device, vk.depthImage, vk.depthImageMemory, 0);

        // Создаём depth image view
        VkImageViewCreateInfo viewCI{};
        viewCI.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCI.image                           = vk.depthImage;
        viewCI.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        viewCI.format                          = vk.depthFormat;
        viewCI.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewCI.subresourceRange.baseMipLevel   = 0;
        viewCI.subresourceRange.levelCount     = 1;
        viewCI.subresourceRange.baseArrayLayer = 0;
        viewCI.subresourceRange.layerCount     = 1;

        if (vkCreateImageView(vk.device, &viewCI, nullptr, &vk.depthImageView) != VK_SUCCESS)
            throw std::runtime_error("Failed to create depth image view.");

        Logger::Info("Depth buffer created (format=" + std::to_string(vk.depthFormat) + ").");
    }
}
