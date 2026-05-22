#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>

namespace RKeng
{
    struct SwapchainSupportDetails
    {
        VkSurfaceCapabilitiesKHR        capabilities{};
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR>   presentModes;
    };

    struct VulkanState
    {
        VkInstance               instance       = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
        VkSurfaceKHR             surface        = VK_NULL_HANDLE;
        VkPhysicalDevice         physicalDevice = VK_NULL_HANDLE;
        VkDevice                 device         = VK_NULL_HANDLE;
        VkQueue                  graphicsQueue  = VK_NULL_HANDLE;
        VkQueue                  presentQueue   = VK_NULL_HANDLE;
        uint32_t                 graphicsQueueFamily = 0;
        uint32_t                 presentQueueFamily  = 0;

        VkSwapchainKHR           swapchain      = VK_NULL_HANDLE;
        std::vector<VkImage>     scImages;
        std::vector<VkImageView> scImageViews;
        VkFormat                 scFormat{};
        VkExtent2D               scExtent{};

        VkRenderPass             renderPass     = VK_NULL_HANDLE;
        VkDescriptorSetLayout    descSetLayout  = VK_NULL_HANDLE;
        VkPipelineLayout         pipelineLayout = VK_NULL_HANDLE;
        VkPipeline               pipeline       = VK_NULL_HANDLE;

        std::vector<VkFramebuffer>   framebuffers;
        VkCommandPool                commandPool = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> commandBuffers;

        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

        // Семафоры acquire: по числу образов swapchain (size = scImages.size()),
        // индексируются через acquireIndex (round-robin), НЕ через currentFrame.
        // Это исключает semaphore-reuse warning: каждый образ имеет свой семафор,
        // presentation гарантированно освобождает его до следующего acquire того же image.
        // renderFinished и inFlightFences — по MAX_FRAMES_IN_FLIGHT (CPU-кадры).
        std::vector<VkSemaphore> imageAvailableSemaphores; // size = scImages.size()
        std::vector<VkSemaphore> renderFinishedSemaphores; // size = MAX_FRAMES_IN_FLIGHT
        std::vector<VkFence>     inFlightFences;           // size = MAX_FRAMES_IN_FLIGHT

        uint32_t acquireIndex = 0; // round-robin по imageAvailableSemaphores

        // Для каждого образа swapchain — какой fence его использует сейчас
        // Нужно чтобы не начать писать в image пока предыдущий кадр его рендерит
        std::vector<VkFence>     imageInFlight; // size = scImages.size(), initially VK_NULL_HANDLE

        uint32_t currentFrame = 0;

        // Vertex/Index buffers (комната)
        VkBuffer       vertexBuffer       = VK_NULL_HANDLE;
        VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
        VkBuffer       indexBuffer        = VK_NULL_HANDLE;
        VkDeviceMemory indexBufferMemory  = VK_NULL_HANDLE;
        uint32_t       indexCount         = 0;

        // Uniform buffers (per-frame)
        std::vector<VkBuffer>       uniformBuffers;
        std::vector<VkDeviceMemory> uniformBuffersMemory;
        std::vector<void*>          uniformBuffersMapped;

        // Descriptor pool/sets
        VkDescriptorPool             descriptorPool = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> descriptorSets;

        uint32_t windowWidth  = 1920;
        uint32_t windowHeight = 1080;

        // Флаг пересоздания swapchain — выставляется при VK_SUBOPTIMAL_KHR / OUT_OF_DATE
        // и обрабатывается в начале следующего DrawFrame.
        bool swapchainDirty = false;

        // Depth buffer
        VkImage        depthImage       = VK_NULL_HANDLE;
        VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
        VkImageView    depthImageView   = VK_NULL_HANDLE;
        VkFormat       depthFormat      = VK_FORMAT_UNDEFINED;
    };

    VulkanState& GetVulkanState();
}
