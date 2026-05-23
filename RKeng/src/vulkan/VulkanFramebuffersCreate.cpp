#include "VulkanFramebuffersCreate.h"
#include "../utils/Logger.h"
#include <array>
#include <stdexcept>

namespace RKeng::VulkanFramebuffersCreate
{
    void Run(VulkanState& vk)
    {
        vk.framebuffers.resize(vk.scImageViews.size());

        for (size_t i = 0; i < vk.scImageViews.size(); i++)
        {
            std::array<VkImageView, 2> attachments = { vk.scImageViews[i], vk.depthImageView };

            VkFramebufferCreateInfo ci{};
            ci.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            ci.renderPass      = vk.renderPass;
            ci.attachmentCount = static_cast<uint32_t>(attachments.size());
            ci.pAttachments    = attachments.data();
            ci.width           = vk.scExtent.width;
            ci.height          = vk.scExtent.height;
            ci.layers          = 1;

            if (vkCreateFramebuffer(vk.device, &ci, nullptr, &vk.framebuffers[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create framebuffer.");
        }

        Logger::Info("Framebuffers created: " + std::to_string(vk.framebuffers.size()));
    }
}
