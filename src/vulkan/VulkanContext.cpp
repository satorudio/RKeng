#include "VulkanContext.h"
#include "VulkanState.h"
#include "VulkanInstanceCreate.h"
#include "VulkanSurfaceCreate.h"
#include "VulkanDeviceSelect.h"
#include "VulkanSwapchainCreate.h"
#include "VulkanDepthCreate.h"
#include "VulkanRenderPassCreate.h"
#include "VulkanFramebuffersCreate.h"
#include "VulkanCommandPoolCreate.h"
#include "VulkanBufferCreate.h"
#include "VulkanDescriptorCreate.h"
#include "VulkanPipelineCreate.h"
#include "VulkanSyncCreate.h"
#include "VulkanFrameDraw.h"
#include "VulkanDestroy.h"
#include "../window/WindowState.h"
#include "../utils/Logger.h"

namespace RKeng::VulkanContext
{
    void Init()
    {
        auto& vk  = GetVulkanState();
        auto& win = GetWindowState();

        VulkanInstanceCreate::Run(vk);
        VulkanSurfaceCreate::Run(vk, win);
        VulkanDeviceSelect::Run(vk);
        VulkanSwapchainCreate::Run(vk);
        VulkanDepthCreate::Run(vk);        // depth image до render pass (нужен формат)
        VulkanRenderPassCreate::Run(vk);
        VulkanFramebuffersCreate::Run(vk);
        VulkanCommandPoolCreate::Run(vk);
        VulkanBufferCreate::Run(vk);       // буферы до дескрипторов
        VulkanDescriptorCreate::Run(vk);   // дескрипторы до пайплайна
        VulkanPipelineCreate::Run(vk);
        VulkanSyncCreate::Run(vk);
        VulkanSyncCreate::InitImageInFlight(vk);  // инициализируем после swapchain

        Logger::Info("VulkanContext fully initialized.");
    }

    void DrawFrame()
    {
        VulkanFrameDraw::Run(GetVulkanState());
    }

    void Shutdown()
    {
        VulkanDestroy::Run(GetVulkanState());
    }
}
