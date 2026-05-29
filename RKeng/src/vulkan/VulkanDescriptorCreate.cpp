#include "VulkanDescriptorCreate.h"
#include "../utils/Logger.h"
#include <stdexcept>

namespace RKeng::VulkanDescriptorCreate
{
    void Run(VulkanState& vk)
    {
        // ── Descriptor Set Layout ─────────────────────────────────────────────
        VkDescriptorSetLayoutBinding uboBinding{};
        uboBinding.binding         = 0;
        uboBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboBinding.descriptorCount = 1;
        // UBO используется и в vertex (view/proj), и во fragment (lighting) шейдерах.
        uboBinding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutCI{};
        layoutCI.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutCI.bindingCount = 1;
        layoutCI.pBindings    = &uboBinding;
        if (vkCreateDescriptorSetLayout(vk.device, &layoutCI, nullptr, &vk.descSetLayout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create descriptor set layout.");

        // ── Descriptor Pool ───────────────────────────────────────────────────
        VkDescriptorPoolSize poolSize{};
        poolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = static_cast<uint32_t>(VulkanState::MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo poolCI{};
        poolCI.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolCI.poolSizeCount = 1;
        poolCI.pPoolSizes    = &poolSize;
        poolCI.maxSets       = static_cast<uint32_t>(VulkanState::MAX_FRAMES_IN_FLIGHT);
        if (vkCreateDescriptorPool(vk.device, &poolCI, nullptr, &vk.descriptorPool) != VK_SUCCESS)
            throw std::runtime_error("Failed to create descriptor pool.");

        // ── Descriptor Sets ───────────────────────────────────────────────────
        std::vector<VkDescriptorSetLayout> layouts(VulkanState::MAX_FRAMES_IN_FLIGHT, vk.descSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool     = vk.descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(VulkanState::MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts        = layouts.data();
        vk.descriptorSets.resize(VulkanState::MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(vk.device, &allocInfo, vk.descriptorSets.data()) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate descriptor sets.");

        for (int i = 0; i < VulkanState::MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufInfo{};
            bufInfo.buffer = vk.uniformBuffers[i];
            bufInfo.offset = 0;
            // Структура UBO в шейдере: mat4 view (64) + mat4 proj (64) +
            // vec3 sunDir (16, std140) + vec3 sunColor (16) + vec3 ambientColor (16) = 176 байт.
            bufInfo.range  = sizeof(float) * (16 + 16 + 4 + 4 + 4); // 176 bytes

            VkWriteDescriptorSet write{};
            write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet          = vk.descriptorSets[i];
            write.dstBinding      = 0;
            write.descriptorCount = 1;
            write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write.pBufferInfo     = &bufInfo;
            vkUpdateDescriptorSets(vk.device, 1, &write, 0, nullptr);
        }

        Logger::Info("Descriptors created.");
    }
}
