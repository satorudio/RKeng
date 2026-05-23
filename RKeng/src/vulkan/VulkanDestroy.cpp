#include "VulkanDestroy.h"
#include "VulkanFrameDraw.h"
#include "../utils/Logger.h"
#include <string>

namespace RKeng::VulkanDestroy
{
    void Run(VulkanState& vk)
    {
        // Ждём пока GPU закончит все операции
        if (vk.device)
        {
            VkResult res = vkDeviceWaitIdle(vk.device);
            if (res != VK_SUCCESS)
                Logger::Warn("[VulkanDestroy] vkDeviceWaitIdle returned " +
                             std::to_string(static_cast<int>(res)));
        }
        else
        {
            Logger::Warn("[VulkanDestroy] device is null, skipping destroy");
            return;
        }

        // Воксельные буферы (до уничтожения device)
        VulkanFrameDraw::DestroyWallBuffers(vk);

        // Sync objects
        for (size_t i = 0; i < vk.imageAvailableSemaphores.size(); i++)
            vkDestroySemaphore(vk.device, vk.imageAvailableSemaphores[i], nullptr);
        for (int i = 0; i < VulkanState::MAX_FRAMES_IN_FLIGHT; i++)
        {
            if (vk.renderFinishedSemaphores.size() > (size_t)i) vkDestroySemaphore(vk.device, vk.renderFinishedSemaphores[i], nullptr);
            if (vk.inFlightFences.size()           > (size_t)i) vkDestroyFence    (vk.device, vk.inFlightFences[i],           nullptr);
        }

        // Uniform buffers (per-frame)
        for (size_t i = 0; i < vk.uniformBuffers.size(); i++)
        {
            if (vk.uniformBuffers[i])       vkDestroyBuffer(vk.device, vk.uniformBuffers[i], nullptr);
            if (vk.uniformBuffersMemory[i]) vkFreeMemory   (vk.device, vk.uniformBuffersMemory[i], nullptr);
        }
        vk.uniformBuffers.clear();
        vk.uniformBuffersMemory.clear();
        vk.uniformBuffersMapped.clear();

        // Descriptor pool (неявно освобождает все descriptor sets)
        if (vk.descriptorPool)  vkDestroyDescriptorPool(vk.device, vk.descriptorPool, nullptr);

        // Descriptor set layout
        if (vk.descSetLayout)   vkDestroyDescriptorSetLayout(vk.device, vk.descSetLayout, nullptr);

        // Vertex / Index buffers (комната)
        if (vk.indexBuffer)        vkDestroyBuffer(vk.device, vk.indexBuffer,        nullptr);
        if (vk.indexBufferMemory)  vkFreeMemory   (vk.device, vk.indexBufferMemory,  nullptr);
        if (vk.vertexBuffer)       vkDestroyBuffer(vk.device, vk.vertexBuffer,       nullptr);
        if (vk.vertexBufferMemory) vkFreeMemory   (vk.device, vk.vertexBufferMemory, nullptr);

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
