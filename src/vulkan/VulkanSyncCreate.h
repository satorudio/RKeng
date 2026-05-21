#pragma once
#include "VulkanState.h"

namespace RKeng::VulkanSyncCreate
{
    void Run(VulkanState& vk);
    void InitImageInFlight(VulkanState& vk);  // вызвать после создания swapchain
}
