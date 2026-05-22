#include "VulkanSwapchainCreate.h"
#include "../utils/Logger.h"
#include <vector>
#include <algorithm>
#include <stdexcept>

namespace RKeng::VulkanSwapchainCreate
{
    static SwapchainSupportDetails QuerySupport(VkPhysicalDevice dev, VkSurfaceKHR surface)
    {
        SwapchainSupportDetails d;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surface, &d.capabilities);

        uint32_t fmtCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &fmtCount, nullptr);
        if (fmtCount) { d.formats.resize(fmtCount); vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &fmtCount, d.formats.data()); }

        uint32_t pmCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &pmCount, nullptr);
        if (pmCount) { d.presentModes.resize(pmCount); vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &pmCount, d.presentModes.data()); }

        return d;
    }

    static VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
    {
        for (const auto& f : formats)
            if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return f;
        return formats[0];
    }

    static VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& modes)
    {
        // Mailbox = triple buffering, без vsync и без тиринга — лучший вариант
        for (const auto& m : modes)
            if (m == VK_PRESENT_MODE_MAILBOX_KHR) return m;
        // Immediate = без vsync, возможен тиринг, но максимальный fps
        for (const auto& m : modes)
            if (m == VK_PRESENT_MODE_IMMEDIATE_KHR) return m;
        // FIFO = vsync, гарантированно поддерживается — последний резерв
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    static VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& caps, uint32_t w, uint32_t h)
    {
        if (caps.currentExtent.width != UINT32_MAX) return caps.currentExtent;
        return {
            std::clamp(w, caps.minImageExtent.width,  caps.maxImageExtent.width),
            std::clamp(h, caps.minImageExtent.height, caps.maxImageExtent.height)
        };
    }

    void Run(VulkanState& vk)
    {
        auto support = QuerySupport(vk.physicalDevice, vk.surface);
        auto fmt     = ChooseSurfaceFormat(support.formats);
        auto pm      = ChoosePresentMode(support.presentModes);
        auto extent  = ChooseExtent(support.capabilities, vk.windowWidth, vk.windowHeight);

        uint32_t imageCount = support.capabilities.minImageCount + 1;
        if (support.capabilities.maxImageCount > 0)
            imageCount = std::min(imageCount, support.capabilities.maxImageCount);

        VkSwapchainCreateInfoKHR ci{};
        ci.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        ci.surface          = vk.surface;
        ci.minImageCount    = imageCount;
        ci.imageFormat      = fmt.format;
        ci.imageColorSpace  = fmt.colorSpace;
        ci.imageExtent      = extent;
        ci.imageArrayLayers = 1;
        ci.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t queueFamilies[] = { vk.graphicsQueueFamily, vk.presentQueueFamily };
        if (vk.graphicsQueueFamily != vk.presentQueueFamily)
        {
            ci.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
            ci.queueFamilyIndexCount = 2;
            ci.pQueueFamilyIndices   = queueFamilies;
        }
        else
        {
            ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        ci.preTransform   = support.capabilities.currentTransform;
        ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        ci.presentMode    = pm;
        ci.clipped        = VK_TRUE;
        ci.oldSwapchain   = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(vk.device, &ci, nullptr, &vk.swapchain) != VK_SUCCESS)
            throw std::runtime_error("Failed to create swapchain.");

        vk.scFormat = fmt.format;
        vk.scExtent = extent;

        // Получаем images
        uint32_t imgCount = 0;
        vkGetSwapchainImagesKHR(vk.device, vk.swapchain, &imgCount, nullptr);
        vk.scImages.resize(imgCount);
        vkGetSwapchainImagesKHR(vk.device, vk.swapchain, &imgCount, vk.scImages.data());

        // Создаём image views
        vk.scImageViews.resize(imgCount);
        for (uint32_t i = 0; i < imgCount; i++)
        {
            VkImageViewCreateInfo vci{};
            vci.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            vci.image                           = vk.scImages[i];
            vci.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
            vci.format                          = vk.scFormat;
            vci.components                      = { VK_COMPONENT_SWIZZLE_IDENTITY,
                                                    VK_COMPONENT_SWIZZLE_IDENTITY,
                                                    VK_COMPONENT_SWIZZLE_IDENTITY,
                                                    VK_COMPONENT_SWIZZLE_IDENTITY };
            vci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            vci.subresourceRange.baseMipLevel   = 0;
            vci.subresourceRange.levelCount     = 1;
            vci.subresourceRange.baseArrayLayer = 0;
            vci.subresourceRange.layerCount     = 1;

            if (vkCreateImageView(vk.device, &vci, nullptr, &vk.scImageViews[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create image view.");
        }

        Logger::Info("Swapchain created (" + std::to_string(extent.width)
                   + "x" + std::to_string(extent.height) + ").");
    }
}
