#pragma once
#include "VulkanState.h"

// Одна задача: создать VkInstance и, в Debug-режиме, validation layers + messenger.

namespace RKeng::VulkanInstanceCreate
{
    void Run(VulkanState& vk);
}
