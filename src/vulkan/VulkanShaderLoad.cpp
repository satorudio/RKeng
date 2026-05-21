#include "VulkanShaderLoad.h"
#include "../utils/Logger.h"
#include <fstream>
#include <stdexcept>
#include <vector>

namespace RKeng::VulkanShaderLoad
{
    VkShaderModule Run(VulkanState& vk, const std::string& path)
    {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file.is_open())
            throw std::runtime_error("Failed to open shader: " + path);

        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), static_cast<std::streamsize>(fileSize));

        VkShaderModuleCreateInfo ci{};
        ci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        ci.codeSize = buffer.size();
        ci.pCode    = reinterpret_cast<const uint32_t*>(buffer.data());

        VkShaderModule module;
        if (vkCreateShaderModule(vk.device, &ci, nullptr, &module) != VK_SUCCESS)
            throw std::runtime_error("Failed to create shader module: " + path);

        Logger::Info("Shader loaded: " + path);
        return module;
    }
}
