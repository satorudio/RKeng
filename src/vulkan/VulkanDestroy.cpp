#include "VulkanDestroy.h"
#include "VulkanFrameDraw.h"
#include "../utils/Logger.h"

namespace RKeng::VulkanDestroy
{
    void Run(VulkanState& vk)
    {
        // Ждём пока GPU закончит все операции
        if (vk.device) vkDeviceWaitIdle(vk.device);

        // Воксельные буферы (до уничтожения device)
        VulkanFrameDraw::DestroyWallBuffers(vk);

        // Sync objects
        for (int i = 0; i < VulkanState::MAX_FRAMES_IN_FLIGHT; i++)
        {
            if (vk.renderFinishedSemaphores.size()  > (size_t)i) vkDestroySemaphore(vk.device, vk.renderFinishedSemaphores[i],  nullptr);
            if (vk.imageAvailableSemaphores.size()  > (size_t)i) vkDestroySemaphore(vk.device, vk.imageAvailableSemaphores[i],  nullptr);
            if (vk.inFlightFences.size()            > (size_t)i) vkDestroyFence    (vk.device, vk.inFlightFences[i],            nullptr);
        }

        if (vk.commandPool)  vkDestroyCommandPool(vk.device, vk.commandPool, nullptr);

        for (auto fb : vk.framebuffers)    vkDestroyFramebuffer(vk.device, fb, nullptr);

        // Depth buffer
        if (vk.depthImageView)   vkDestroyImageView(vk.device, vk.depthImageView, nullptr);
        if (vk.depthImage)       vkDestroyImage(vk.device, vk.depthImage, nullptr);
        if (vk.depthImageMemory) vkFreeMemory(vk.device, vk.depthImageMemory, nullptr);

        if  (vk.pipeline)                  vkDestroyPipeline(vk.device, vk.pipeline, nullptr);
        if  (vk.pipelineLayout)            vkDestroyPipelineLayout(vk.device, vk.pipelineLayout, nullptr);
        if  (vk.renderPass)                vkDestroyRenderPass(vk.device, vk.renderPass, nullptr);
        for (auto iv : vk.scImageViews)    vkDestroyImageView(vk.device, iv, nullptr);
        if  (vk.swapchain)                 vkDestroySwapchainKHR(vk.device, vk.swapchain, nullptr);
        if  (vk.device)                    vkDestroyDevice(vk.device, nullptr);

        // Debug messenger
        if (vk.debugMessenger)
        {
            auto fn = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(vk.instance, "vkDestroyDebugUtilsMessengerEXT"));
            if (fn) fn(vk.instance, vk.debugMessenger, nullptr);
        }

        if (vk.surface)  vkDestroySurfaceKHR(vk.instance, vk.surface, nullptr);
        if (vk.instance) vkDestroyInstance(vk.instance, nullptr);

        Logger::Info("Vulkan destroyed.");
    }
}
