#include "scene/CarGpuBuffer.h"
#include "VulkanBufferCreate.h"
#include "Logger.h"
#include <cstring>

namespace RKeng::CarGpuBuffer
{
    static void DestroyBuffers(VulkanState& vk, CarBuffers& buf)
    {
        if (buf.vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(vk.device, buf.vertexBuffer, nullptr);
            vkFreeMemory(vk.device, buf.vertexBufferMemory, nullptr);
            buf.vertexBuffer = VK_NULL_HANDLE;
        }
        if (buf.indexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(vk.device, buf.indexBuffer, nullptr);
            vkFreeMemory(vk.device, buf.indexBufferMemory, nullptr);
            buf.indexBuffer = VK_NULL_HANDLE;
        }
        buf.valid = false;
        buf.indexCount = 0;
    }

    void UploadIfDirty(VulkanState& vk, CarState& car, CarBuffers& buf)
    {
        if (!car.meshDirty) return;
        car.meshDirty = false;

        DestroyBuffers(vk, buf);

        if (car.meshVertices.empty() || car.meshIndices.empty()) return;

        // Vertex buffer
        // stride = 9 floats: pos(3) + color(3) + normal(3)
        VkDeviceSize vbSize = sizeof(float) * car.meshVertices.size();
        VkBuffer stagV; VkDeviceMemory stagVM;
        VulkanBufferCreate::CreateBuffer(vk, vbSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagV, stagVM);
        void* dv; vkMapMemory(vk.device, stagVM, 0, vbSize, 0, &dv);
        memcpy(dv, car.meshVertices.data(), vbSize);
        vkUnmapMemory(vk.device, stagVM);

        VulkanBufferCreate::CreateBuffer(vk, vbSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            buf.vertexBuffer, buf.vertexBufferMemory);
        VulkanBufferCreate::CopyBuffer(vk, stagV, buf.vertexBuffer, vbSize);
        vkDestroyBuffer(vk.device, stagV, nullptr);
        vkFreeMemory(vk.device, stagVM, nullptr);

        // Index buffer
        VkDeviceSize ibSize = sizeof(uint32_t) * car.meshIndices.size();
        VkBuffer stagI; VkDeviceMemory stagIM;
        VulkanBufferCreate::CreateBuffer(vk, ibSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagI, stagIM);
        void* di; vkMapMemory(vk.device, stagIM, 0, ibSize, 0, &di);
        memcpy(di, car.meshIndices.data(), ibSize);
        vkUnmapMemory(vk.device, stagIM);

        VulkanBufferCreate::CreateBuffer(vk, ibSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            buf.indexBuffer, buf.indexBufferMemory);
        VulkanBufferCreate::CopyBuffer(vk, stagI, buf.indexBuffer, ibSize);
        vkDestroyBuffer(vk.device, stagI, nullptr);
        vkFreeMemory(vk.device, stagIM, nullptr);

        buf.indexCount = (uint32_t)car.meshIndices.size();
        buf.valid = true;
    }

    void Destroy(VulkanState& vk, CarBuffers& buf)
    {
        DestroyBuffers(vk, buf);
    }
}
