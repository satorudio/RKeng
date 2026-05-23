#include "VulkanDeviceSelect.h"
#include "../utils/Logger.h"
#include <vector>
#include <set>
#include <stdexcept>
#include <string>
#include <optional>

namespace RKeng::VulkanDeviceSelect
{
    static const std::vector<const char*> k_DeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphics;
        std::optional<uint32_t> present;
        bool IsComplete() const { return graphics.has_value() && present.has_value(); }
    };

    static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice dev, VkSurfaceKHR surface)
    {
        QueueFamilyIndices idx;
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, nullptr);
        std::vector<VkQueueFamilyProperties> families(count);
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, families.data());

        for (uint32_t i = 0; i < count; i++)
        {
            if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                idx.graphics = i;

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &presentSupport);
            if (presentSupport) idx.present = i;

            if (idx.IsComplete()) break;
        }
        return idx;
    }

    static bool CheckDeviceExtensionSupport(VkPhysicalDevice dev)
    {
        uint32_t count = 0;
        vkEnumerateDeviceExtensionProperties(dev, nullptr, &count, nullptr);
        std::vector<VkExtensionProperties> available(count);
        vkEnumerateDeviceExtensionProperties(dev, nullptr, &count, available.data());

        std::set<std::string> required(k_DeviceExtensions.begin(), k_DeviceExtensions.end());
        for (const auto& ext : available) required.erase(ext.extensionName);
        return required.empty();
    }

    static int ScoreDevice(VkPhysicalDevice dev, VkSurfaceKHR surface)
    {
        if (!FindQueueFamilies(dev, surface).IsComplete())  return -1;
        if (!CheckDeviceExtensionSupport(dev))              return -1;

        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(dev, &props);

        int score = 0;
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)   score += 1000;
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) score += 100;
        score += static_cast<int>(props.limits.maxImageDimension2D / 1000);
        return score;
    }

    void Run(VulkanState& vk)
    {
        // --- Выбор физического устройства ---
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(vk.instance, &deviceCount, nullptr);
        if (deviceCount == 0) throw std::runtime_error("No Vulkan-capable GPU found.");

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(vk.instance, &deviceCount, devices.data());

        int bestScore = -1;
        for (auto dev : devices)
        {
            int s = ScoreDevice(dev, vk.surface);
            if (s > bestScore) { bestScore = s; vk.physicalDevice = dev; }
        }

        if (vk.physicalDevice == VK_NULL_HANDLE)
            throw std::runtime_error("No suitable GPU found.");

        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(vk.physicalDevice, &props);
        Logger::Info(std::string("Selected GPU: ") + props.deviceName);

        // --- Логическое устройство ---
        auto idx = FindQueueFamilies(vk.physicalDevice, vk.surface);
        vk.graphicsQueueFamily = idx.graphics.value();
        vk.presentQueueFamily  = idx.present.value();

        std::set<uint32_t> uniqueQueues = { vk.graphicsQueueFamily, vk.presentQueueFamily };
        std::vector<VkDeviceQueueCreateInfo> queueCIs;
        float priority = 1.0f;
        for (uint32_t qf : uniqueQueues)
        {
            VkDeviceQueueCreateInfo qci{};
            qci.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            qci.queueFamilyIndex = qf;
            qci.queueCount       = 1;
            qci.pQueuePriorities = &priority;
            queueCIs.push_back(qci);
        }

        VkPhysicalDeviceFeatures features{};
        features.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo dci{};
        dci.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        dci.queueCreateInfoCount    = static_cast<uint32_t>(queueCIs.size());
        dci.pQueueCreateInfos       = queueCIs.data();
        dci.enabledExtensionCount   = static_cast<uint32_t>(k_DeviceExtensions.size());
        dci.ppEnabledExtensionNames = k_DeviceExtensions.data();
        dci.pEnabledFeatures        = &features;

        if (vkCreateDevice(vk.physicalDevice, &dci, nullptr, &vk.device) != VK_SUCCESS)
            throw std::runtime_error("Failed to create logical device.");

        vkGetDeviceQueue(vk.device, vk.graphicsQueueFamily, 0, &vk.graphicsQueue);
        vkGetDeviceQueue(vk.device, vk.presentQueueFamily,  0, &vk.presentQueue);

        Logger::Info("Logical device created.");
    }
}
