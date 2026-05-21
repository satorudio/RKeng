#include "VulkanFrameDraw.h"
#include "VulkanSwapchainRecreate.h"
#include "../utils/Logger.h"
#include "../core/SceneState.h"
#include "VoxelWallBuffer.h"
#include "../vulkan/VulkanBufferCreate.h"
#include "../math/Camera.h"
#include <array>
#include <stdexcept>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace RKeng::VulkanFrameDraw
{
    struct UBO { glm::mat4 model, view, proj; };

    static std::vector<VoxelWallBuffer::WallGpuBuffers> s_wallBuffers;

    // Generic меш сцены (машина, объекты — что угодно из DLL)
    struct SceneMeshBuffers
    {
        VkBuffer       vertexBuffer       = VK_NULL_HANDLE;
        VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
        VkBuffer       indexBuffer        = VK_NULL_HANDLE;
        VkDeviceMemory indexBufferMemory  = VK_NULL_HANDLE;
        uint32_t       indexCount         = 0;
        bool           valid              = false;
    };
    static SceneMeshBuffers s_sceneMeshBuf;

    static void DestroySceneMeshBuffers(VulkanState& vk)
    {
        if (s_sceneMeshBuf.vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(vk.device, s_sceneMeshBuf.vertexBuffer, nullptr);
            vkFreeMemory(vk.device, s_sceneMeshBuf.vertexBufferMemory, nullptr);
            s_sceneMeshBuf.vertexBuffer = VK_NULL_HANDLE;
        }
        if (s_sceneMeshBuf.indexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(vk.device, s_sceneMeshBuf.indexBuffer, nullptr);
            vkFreeMemory(vk.device, s_sceneMeshBuf.indexBufferMemory, nullptr);
            s_sceneMeshBuf.indexBuffer = VK_NULL_HANDLE;
        }
        s_sceneMeshBuf.valid = false;
        s_sceneMeshBuf.indexCount = 0;
    }

    static void UploadSceneMeshIfDirty(VulkanState& vk, SceneMesh& mesh)
    {
        if (!mesh.dirty) return;
        mesh.dirty = false;

        DestroySceneMeshBuffers(vk);

        if (mesh.vertices.empty() || mesh.indices.empty()) return;

        // RAII guard для staging-буфера: гарантирует освобождение ресурсов
        // даже если CreateBuffer для device-local буфера завершится с ошибкой.
        struct StagingGuard {
            VkDevice       device = VK_NULL_HANDLE;
            VkBuffer       buf    = VK_NULL_HANDLE;
            VkDeviceMemory mem    = VK_NULL_HANDLE;
            ~StagingGuard() {
                if (buf != VK_NULL_HANDLE) vkDestroyBuffer(device, buf, nullptr);
                if (mem != VK_NULL_HANDLE) vkFreeMemory(device, mem, nullptr);
            }
        };

        auto upload = [&](const void* data, VkDeviceSize size,
                          VkBufferUsageFlags usage,
                          VkBuffer& buf, VkDeviceMemory& mem)
        {
            StagingGuard stag{ vk.device };
            VulkanBufferCreate::CreateBuffer(vk, size,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stag.buf, stag.mem);
            void* ptr; vkMapMemory(vk.device, stag.mem, 0, size, 0, &ptr);
            memcpy(ptr, data, size);
            vkUnmapMemory(vk.device, stag.mem);
            VulkanBufferCreate::CreateBuffer(vk, size,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buf, mem);
            VulkanBufferCreate::CopyBuffer(vk, stag.buf, buf, size);
            // stag уничтожается здесь автоматически (деструктор ~StagingGuard)
        };

        VkDeviceSize vbSize = sizeof(float)    * mesh.vertices.size();
        VkDeviceSize ibSize = sizeof(uint32_t) * mesh.indices.size();
        upload(mesh.vertices.data(), vbSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
               s_sceneMeshBuf.vertexBuffer, s_sceneMeshBuf.vertexBufferMemory);
        upload(mesh.indices.data(),  ibSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
               s_sceneMeshBuf.indexBuffer,  s_sceneMeshBuf.indexBufferMemory);
        s_sceneMeshBuf.indexCount = (uint32_t)mesh.indices.size();
        s_sceneMeshBuf.valid = true;
    }

    static void UpdateUBO(VulkanState& vk, uint32_t frame)
    {
        auto& scene  = GetSceneState();
        auto& player = scene.player;
        auto& input  = scene.input;

        float yaw   = glm::radians(input.yaw);
        float pitch = glm::radians(input.pitch);

        glm::vec3 pos = player.worldPos.ToLocal(scene.originShift);
        pos.y += player.currentHeight * 0.85f;

        glm::vec3 front;
        front.x = glm::sin(yaw) * glm::cos(pitch);
        front.y = glm::sin(pitch);
        front.z = -glm::cos(yaw) * glm::cos(pitch);
        front = glm::normalize(front);

        float nearPlane = player.isCrouching ? 0.01f : 0.05f;

        UBO ubo{};
        ubo.model = glm::mat4(1.0f);
        ubo.view  = glm::lookAt(pos, pos + front, glm::vec3(0,1,0));
        ubo.proj  = glm::perspective(glm::radians(90.0f),
                        static_cast<float>(vk.scExtent.width) /
                        static_cast<float>(vk.scExtent.height),
                        nearPlane, 500.0f);
        ubo.proj[1][1] *= -1.0f;

        memcpy(vk.uniformBuffersMapped[frame], &ubo, sizeof(ubo));
    }

    static void RecordCommandBuffer(VulkanState& vk, VkCommandBuffer cmd, uint32_t imageIndex)
    {
        VkCommandBufferBeginInfo beginCI{};
        beginCI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        if (vkBeginCommandBuffer(cmd, &beginCI) != VK_SUCCESS)
            throw std::runtime_error("Failed to begin command buffer.");

        VkRenderPassBeginInfo rpBI{};
        rpBI.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpBI.renderPass        = vk.renderPass;
        rpBI.framebuffer       = vk.framebuffers[imageIndex];
        rpBI.renderArea.offset = {0, 0};
        rpBI.renderArea.extent = vk.scExtent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color        = {{ 0.05f, 0.05f, 0.07f, 1.0f }};
        clearValues[1].depthStencil = { 1.0f, 0 };
        rpBI.clearValueCount = static_cast<uint32_t>(clearValues.size());
        rpBI.pClearValues    = clearValues.data();

        vkCmdBeginRenderPass(cmd, &rpBI, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline);

        VkViewport viewport{};
        viewport.x = 0; viewport.y = 0;
        viewport.width    = static_cast<float>(vk.scExtent.width);
        viewport.height   = static_cast<float>(vk.scExtent.height);
        viewport.minDepth = 0.0f; viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor{{0,0}, vk.scExtent};
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                vk.pipelineLayout, 0, 1,
                                &vk.descriptorSets[vk.currentFrame], 0, nullptr);

        // Identity push constant для статических мешей
        {
            glm::mat4 identity(1.0f);
            vkCmdPushConstants(cmd, vk.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                               0, sizeof(glm::mat4), &identity[0][0]);
        }

        // Статический меш сцены (пол, стены — из VulkanDescriptorCreate)
        if (vk.indexCount > 0)
        {
            VkBuffer vbufs[] = { vk.vertexBuffer };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(cmd, 0, 1, vbufs, offsets);
            vkCmdBindIndexBuffer(cmd, vk.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(cmd, vk.indexCount, 1, 0, 0, 0);
        }

        // Воксельные стены
        for (const auto& gb : s_wallBuffers)
        {
            if (!gb.valid || gb.indexCount == 0) continue;
            VkBuffer vbufs[] = { gb.vertexBuffer };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(cmd, 0, 1, vbufs, offsets);
            vkCmdBindIndexBuffer(cmd, gb.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(cmd, gb.indexCount, 1, 0, 0, 0);
        }

        // Generic меш сцены (машина, etc.) с трансформом из SceneMesh.modelMatrix
        if (s_sceneMeshBuf.valid && s_sceneMeshBuf.indexCount > 0)
        {
            auto& scene = GetSceneState();
            vkCmdPushConstants(cmd, vk.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                               0, sizeof(glm::mat4), &scene.sceneMesh.modelMatrix[0][0]);

            VkBuffer vbufs[] = { s_sceneMeshBuf.vertexBuffer };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(cmd, 0, 1, vbufs, offsets);
            vkCmdBindIndexBuffer(cmd, s_sceneMeshBuf.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(cmd, s_sceneMeshBuf.indexCount, 1, 0, 0, 0);
        }

        vkCmdEndRenderPass(cmd);
        if (vkEndCommandBuffer(cmd) != VK_SUCCESS)
            throw std::runtime_error("Failed to record command buffer.");
    }

    void Run(VulkanState& vk)
    {
        if (vk.swapchainDirty)
        {
            VulkanSwapchainRecreate::Run(vk);
            vk.swapchainDirty = false;
        }

        const uint32_t frame = vk.currentFrame;
        vkWaitForFences(vk.device, 1, &vk.inFlightFences[frame], VK_TRUE, UINT64_MAX);

        // Обновляем воксельные буферы
        auto& scene = GetSceneState();
        bool anyDirty = false;
        for (auto& w : scene.voxelWalls)
            if (w.meshDirty) { anyDirty = true; break; }

        if (anyDirty && vk.inFlightFences.size() > 1)
            vkWaitForFences(vk.device,
                            static_cast<uint32_t>(vk.inFlightFences.size()),
                            vk.inFlightFences.data(), VK_TRUE, UINT64_MAX);
        if (anyDirty)
            VoxelWallBuffer::UploadDirtyWalls(vk, scene.voxelWalls, s_wallBuffers);

        // Обновляем generic меш сцены
        if (scene.sceneMesh.dirty)
        {
            vkWaitForFences(vk.device,
                            static_cast<uint32_t>(vk.inFlightFences.size()),
                            vk.inFlightFences.data(), VK_TRUE, UINT64_MAX);
            UploadSceneMeshIfDirty(vk, scene.sceneMesh);
        }

        uint32_t imageIndex = 0;
        VkResult result = vkAcquireNextImageKHR(
            vk.device, vk.swapchain, UINT64_MAX,
            vk.imageAvailableSemaphores[frame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            Logger::Warn("Swapchain out of date.");
            vk.swapchainDirty = true;
            return;
        }
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            throw std::runtime_error("Failed to acquire swapchain image.");
        if (result == VK_SUBOPTIMAL_KHR)
            vk.swapchainDirty = true;

        if (vk.imageInFlight[imageIndex] != VK_NULL_HANDLE)
            vkWaitForFences(vk.device, 1, &vk.imageInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        vk.imageInFlight[imageIndex] = vk.inFlightFences[frame];

        vkResetFences(vk.device, 1, &vk.inFlightFences[frame]);

        UpdateUBO(vk, frame);

        VkCommandBuffer cmd = vk.commandBuffers[frame];
        vkResetCommandBuffer(cmd, 0);
        RecordCommandBuffer(vk, cmd, imageIndex);

        VkSemaphore waitSems[]   = { vk.imageAvailableSemaphores[frame] };
        VkSemaphore signalSems[] = { vk.renderFinishedSemaphores[frame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkSubmitInfo submitCI{};
        submitCI.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitCI.waitSemaphoreCount   = 1;
        submitCI.pWaitSemaphores      = waitSems;
        submitCI.pWaitDstStageMask    = waitStages;
        submitCI.commandBufferCount   = 1;
        submitCI.pCommandBuffers      = &cmd;
        submitCI.signalSemaphoreCount = 1;
        submitCI.pSignalSemaphores    = signalSems;
        if (vkQueueSubmit(vk.graphicsQueue, 1, &submitCI, vk.inFlightFences[frame]) != VK_SUCCESS)
            throw std::runtime_error("Failed to submit draw command buffer.");

        VkPresentInfoKHR presentCI{};
        presentCI.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentCI.waitSemaphoreCount = 1;
        presentCI.pWaitSemaphores    = signalSems;
        presentCI.swapchainCount     = 1;
        presentCI.pSwapchains        = &vk.swapchain;
        presentCI.pImageIndices      = &imageIndex;

        result = vkQueuePresentKHR(vk.presentQueue, &presentCI);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
            vk.swapchainDirty = true;
        else if (result != VK_SUCCESS)
            throw std::runtime_error("Failed to present swapchain image.");

        vk.currentFrame = (frame + 1) % VulkanState::MAX_FRAMES_IN_FLIGHT;
    }

    void DestroyWallBuffers(VulkanState& vk)
    {
        vkDeviceWaitIdle(vk.device);
        VoxelWallBuffer::DestroyAll(vk, s_wallBuffers);
        DestroySceneMeshBuffers(vk);
    }
}
