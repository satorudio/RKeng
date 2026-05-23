#pragma once
#include "VulkanState.h"

namespace RKeng::VulkanFrameDraw
{
    void Run(VulkanState& vk);
    void DestroyWallBuffers(VulkanState& vk);  // вызвать до VulkanDestroy::Run
}
