#include "VulkanSyncCreate.h"
#include "../utils/Logger.h"
#include <stdexcept>

namespace RKeng::VulkanSyncCreate
{
    void Run(VulkanState& vk)
    {
        const int N = VulkanState::MAX_FRAMES_IN_FLIGHT;
        vk.imageAvailableSemaphores.resize(N);
        vk.renderFinishedSemaphores.resize(N);
        vk.inFlightFences.resize(N);

        VkSemaphoreCreateInfo semCI{};
        semCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceCI{};
        fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (int i = 0; i < N; i++)
        {
            if (vkCreateSemaphore(vk.device, &semCI, nullptr, &vk.imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(vk.device, &semCI, nullptr, &vk.renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence    (vk.device, &fenceCI, nullptr, &vk.inFlightFences[i])         != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create sync objects.");
            }
        }

        Logger::Info("Sync objects created (MAX_FRAMES_IN_FLIGHT=" + std::to_string(N) + ").");
    }

    void InitImageInFlight(VulkanState& vk)
    {
        // Один null-fence на каждый образ swapchain.
        // Заполняется в VulkanFrameDraw::Run() перед каждым submit'ом.
        vk.imageInFlight.assign(vk.scImages.size(), VK_NULL_HANDLE);
    }
}
