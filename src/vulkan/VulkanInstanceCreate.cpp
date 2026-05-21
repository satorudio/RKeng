#include "VulkanInstanceCreate.h"
#include "../utils/Logger.h"
#include <vector>
#include <stdexcept>
#include <cstring>

namespace RKeng::VulkanInstanceCreate
{
    static const std::vector<const char*> k_ValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    static bool CheckValidationLayerSupport()
    {
        uint32_t count = 0;
        vkEnumerateInstanceLayerProperties(&count, nullptr);
        std::vector<VkLayerProperties> available(count);
        vkEnumerateInstanceLayerProperties(&count, available.data());

        for (const char* name : k_ValidationLayers)
        {
            bool found = false;
            for (const auto& layer : available)
                if (strcmp(name, layer.layerName) == 0) { found = true; break; }
            if (!found) return false;
        }
        return true;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
        VkDebugUtilsMessageTypeFlagsEXT             /*type*/,
        const VkDebugUtilsMessengerCallbackDataEXT* data,
        void*                                       /*user*/)
    {
        if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            Logger::Warn(std::string("[Vulkan] ") + data->pMessage);
        return VK_FALSE;
    }

    static void CreateDebugMessenger(VulkanState& vk)
    {
        VkDebugUtilsMessengerCreateInfoEXT ci{};
        ci.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        ci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                           | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        ci.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                           | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                           | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        ci.pfnUserCallback = DebugCallback;

        auto fn = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(vk.instance, "vkCreateDebugUtilsMessengerEXT"));
        if (!fn || fn(vk.instance, &ci, nullptr, &vk.debugMessenger) != VK_SUCCESS)
            Logger::Warn("Failed to create debug messenger.");
    }

    void Run(VulkanState& vk)
    {
#ifdef RK_DEBUG
        const bool enableValidation = true;
#else
        const bool enableValidation = false;
#endif

        if (enableValidation && !CheckValidationLayerSupport())
            throw std::runtime_error("Validation layers requested but not available.");

        VkApplicationInfo appInfo{};
        appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName   = "RKeng";
        appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
        appInfo.pEngineName        = "RKeng";
        appInfo.engineVersion      = VK_MAKE_VERSION(0, 1, 0);
        appInfo.apiVersion         = VK_API_VERSION_1_3;

        // Расширения: минимум для поверхности + debug в Debug-режиме
        std::vector<const char*> extensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef _WIN32
            "VK_KHR_win32_surface",
#elif defined(__linux__)
            "VK_KHR_xcb_surface",
#elif defined(__APPLE__)
            "VK_MVK_macos_surface",
#endif
        };
        if (enableValidation)
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        VkInstanceCreateInfo ci{};
        ci.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        ci.pApplicationInfo        = &appInfo;
        ci.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
        ci.ppEnabledExtensionNames = extensions.data();

        if (enableValidation)
        {
            ci.enabledLayerCount   = static_cast<uint32_t>(k_ValidationLayers.size());
            ci.ppEnabledLayerNames = k_ValidationLayers.data();
        }

        if (vkCreateInstance(&ci, nullptr, &vk.instance) != VK_SUCCESS)
            throw std::runtime_error("Failed to create VkInstance.");

        if (enableValidation)
            CreateDebugMessenger(vk);

        Logger::Info("VkInstance created.");
    }
}
