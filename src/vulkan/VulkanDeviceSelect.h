#pragma once
#include "VulkanState.h"

// Одна задача: выбрать физическое устройство и создать логическое.

namespace RKeng::VulkanDeviceSelect
{
    void Run(VulkanState& vk);
}
