#pragma once
#include "VulkanState.h"
#include <string>

// Одна задача: загрузить .spv файл и создать VkShaderModule.

namespace RKeng::VulkanShaderLoad
{
    VkShaderModule Run(VulkanState& vk, const std::string& path);
}
