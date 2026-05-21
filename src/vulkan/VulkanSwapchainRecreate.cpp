#include "VulkanSwapchainRecreate.h"
#include "VulkanDepthCreate.h"
#include "VulkanFramebuffersCreate.h"
#include "../utils/Logger.h"
#include <stdexcept>

namespace RKeng::VulkanSwapchainRecreate
{
    // -----------------------------------------------------------------------
    // Вспомогательные функции разрушения
    // -----------------------------------------------------------------------

    static void DestroyFramebuffers(VulkanState& vk)
    {
        for (auto fb : vk.framebuffers)
            if (fb != VK_NULL_HANDLE)
                vkDestroyFramebuffer(vk.device, fb, nullptr);
        vk.framebuffers.clear();
    }

    static void DestroyDepth(VulkanState& vk)
    {
        if (vk.depthImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(vk.device, vk.depthImageView, nullptr);
            vk.depthImageView = VK_NULL_HANDLE;
        }
        if (vk.depthImage != VK_NULL_HANDLE)
        {
            vkDestroyImage(vk.device, vk.depthImage, nullptr);
            vk.depthImage = VK_NULL_HANDLE;
        }
        if (vk.depthImageMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(vk.device, vk.depthImageMemory, nullptr);
            vk.depthImageMemory = VK_NULL_HANDLE;
        }
    }

    static void DestroySwapchainImageViews(VulkanState& vk)
    {
        for (auto iv : vk.scImageViews)
            if (iv != VK_NULL_HANDLE)
                vkDestroyImageView(vk.device, iv, nullptr);
        vk.scImageViews.clear();
    }

    static void DestroySwapchain(VulkanState& vk)
    {
        if (vk.swapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(vk.device, vk.swapchain, nullptr);
            vk.swapchain = VK_NULL_HANDLE;
        }
        vk.scImages.clear();
    }

    // -----------------------------------------------------------------------
    // Воссоздание swapchain
    // Логика повторяет VulkanSwapchainCreate::Run, но сохраняет старый
    // swapchain как oldSwapchain для возможной оптимизации драйвером.
    // -----------------------------------------------------------------------

    static void RecreateSwapchain(VulkanState& vk)
    {
        // Запросить актуальные возможности поверхности
        VkSurfaceCapabilitiesKHR caps;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk.physicalDevice, vk.surface, &caps);

        // Extent
        VkExtent2D extent;
        if (caps.currentExtent.width != UINT32_MAX)
        {
            extent = caps.currentExtent;
        }
        else
        {
            extent.width  = std::max(caps.minImageExtent.width,
                            std::min(caps.maxImageExtent.width,  vk.windowWidth));
            extent.height = std::max(caps.minImageExtent.height,
                            std::min(caps.maxImageExtent.height, vk.windowHeight));
        }

        // Количество образов
        uint32_t imageCount = caps.minImageCount + 1;
        if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount)
            imageCount = caps.maxImageCount;

        // Формат — берём уже выбранный ранее (он не меняется при ресайзе)
        VkFormat    format     = vk.scFormat;
        VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR; // стандарт

        // Создаём новый swapchain, указывая старый как oldSwapchain
        VkSwapchainKHR oldSwapchain = vk.swapchain;

        VkSwapchainCreateInfoKHR ci{};
        ci.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        ci.surface          = vk.surface;
        ci.minImageCount    = imageCount;
        ci.imageFormat      = format;
        ci.imageColorSpace  = colorSpace;
        ci.imageExtent      = extent;
        ci.imageArrayLayers = 1;
        ci.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        if (vk.graphicsQueueFamily != vk.presentQueueFamily)
        {
            uint32_t queueFamilies[] = { vk.graphicsQueueFamily, vk.presentQueueFamily };
            ci.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
            ci.queueFamilyIndexCount = 2;
            ci.pQueueFamilyIndices   = queueFamilies;
        }
        else
        {
            ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        ci.preTransform   = caps.currentTransform;
        ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        ci.presentMode    = VK_PRESENT_MODE_FIFO_KHR; // гарантированно поддерживается
        ci.clipped        = VK_TRUE;
        ci.oldSwapchain   = oldSwapchain;

        VkSwapchainKHR newSwapchain = VK_NULL_HANDLE;
        if (vkCreateSwapchainKHR(vk.device, &ci, nullptr, &newSwapchain) != VK_SUCCESS)
            throw std::runtime_error("VulkanSwapchainRecreate: failed to recreate swapchain.");

        // Только теперь уничтожаем старый (после создания нового)
        if (oldSwapchain != VK_NULL_HANDLE)
            vkDestroySwapchainKHR(vk.device, oldSwapchain, nullptr);

        vk.swapchain = newSwapchain;
        vk.scExtent  = extent;

        // Получаем образы
        uint32_t count = 0;
        vkGetSwapchainImagesKHR(vk.device, vk.swapchain, &count, nullptr);
        vk.scImages.resize(count);
        vkGetSwapchainImagesKHR(vk.device, vk.swapchain, &count, vk.scImages.data());

        Logger::Info("VulkanSwapchainRecreate: swapchain recreated ("
                     + std::to_string(extent.width) + "x" + std::to_string(extent.height) + ").");
    }

    // -----------------------------------------------------------------------
    // Воссоздание ImageViews для образов swapchain
    // -----------------------------------------------------------------------

    static void RecreateImageViews(VulkanState& vk)
    {
        vk.scImageViews.resize(vk.scImages.size());

        for (size_t i = 0; i < vk.scImages.size(); i++)
        {
            VkImageViewCreateInfo ci{};
            ci.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            ci.image                           = vk.scImages[i];
            ci.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
            ci.format                          = vk.scFormat;
            ci.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            ci.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            ci.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            ci.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            ci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            ci.subresourceRange.baseMipLevel   = 0;
            ci.subresourceRange.levelCount     = 1;
            ci.subresourceRange.baseArrayLayer = 0;
            ci.subresourceRange.layerCount     = 1;

            if (vkCreateImageView(vk.device, &ci, nullptr, &vk.scImageViews[i]) != VK_SUCCESS)
                throw std::runtime_error("VulkanSwapchainRecreate: failed to create image view.");
        }

        Logger::Info("VulkanSwapchainRecreate: image views recreated ("
                     + std::to_string(vk.scImageViews.size()) + ").");
    }

    // -----------------------------------------------------------------------
    // Обновляем imageInFlight под новое количество образов
    // -----------------------------------------------------------------------

    static void ResetImageInFlight(VulkanState& vk)
    {
        vk.imageInFlight.assign(vk.scImages.size(), VK_NULL_HANDLE);
    }

    // -----------------------------------------------------------------------
    // Точка входа
    // -----------------------------------------------------------------------

    void Run(VulkanState& vk)
    {
        Logger::Info("VulkanSwapchainRecreate: starting...");

        // 1. Ждём завершения всех GPU-операций
        vkDeviceWaitIdle(vk.device);

        // 2. Уничтожаем зависимые ресурсы в правильном порядке
        DestroyFramebuffers(vk);
        DestroyDepth(vk);
        DestroySwapchainImageViews(vk);
        // Swapchain уничтожается внутри RecreateSwapchain после создания нового

        // 3. Пересоздаём swapchain (со старым в качестве oldSwapchain)
        RecreateSwapchain(vk);

        // 4. Пересоздаём imageviews
        RecreateImageViews(vk);

        // 5. Пересоздаём depth buffer
        VulkanDepthCreate::Run(vk);

        // 6. Пересоздаём framebuffers
        VulkanFramebuffersCreate::Run(vk);

        // 7. Сбрасываем imageInFlight под новый размер
        ResetImageInFlight(vk);

        Logger::Info("VulkanSwapchainRecreate: done.");
    }
}
