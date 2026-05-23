#include "VulkanFrameDraw.h"
#include "VulkanSwapchainRecreate.h"
#include "../utils/Logger.h"
#include "../core/SceneState.h"
#include "VoxelWallBuffer.h"
#include "../vulkan/VulkanBufferCreate.h"
#include "../math/Camera.h"
#include "../math/Frustum.h"
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

    // ── Instance buffer для кубов ─────────────────────────────────────────
    // Layout per instance: mat4(16 floats) + vec3 color(3) + float wire(1) = 20 floats
    struct InstanceBuffer
    {
        VkBuffer       buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        uint32_t       count  = 0;
        bool           valid  = false;
    };
    static InstanceBuffer s_instanceBuf;

    // Dummy instance буфер: один инстанс с identity матрицей.
    // Биндится на binding 1 при non-instanced draws чтобы GPU не читал мусор.
    // instanceM[3][3] = 1.0 → шейдер берёт ubo.model * I = ubo.model (корректно).
    struct DummyInstanceBuf
    {
        VkBuffer       buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        bool           valid  = false;
    };
    static DummyInstanceBuf s_dummyInstance;

    // Единственный куб 1x1x1 — shared geometry для всех инстансов
    struct CubeGeo
    {
        VkBuffer       vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexMem    = VK_NULL_HANDLE;
        VkBuffer       indexBuffer  = VK_NULL_HANDLE;
        VkDeviceMemory indexMem     = VK_NULL_HANDLE;
        uint32_t       indexCount   = 0;
        bool           built        = false;
    };
    static CubeGeo s_cubeGeo;

    static void BuildUnitCubeGeo(VulkanState& vk)
    {
        if (s_cubeGeo.built) return;
        // pos+normal+color per vertex (9 floats), color будет overridden instanceColor
        struct V { float p[3]; float n[3]; float c[3]; };
        const float W = 0.5f;
        // 6 граней, 4 вершины каждая
        static const V verts[] = {
            // +Z
            {{-W,-W, W},{0,0,1},{1,1,1}}, {{ W,-W, W},{0,0,1},{1,1,1}},
            {{ W, W, W},{0,0,1},{1,1,1}}, {{-W, W, W},{0,0,1},{1,1,1}},
            // -Z
            {{ W,-W,-W},{0,0,-1},{1,1,1}}, {{-W,-W,-W},{0,0,-1},{1,1,1}},
            {{-W, W,-W},{0,0,-1},{1,1,1}}, {{ W, W,-W},{0,0,-1},{1,1,1}},
            // +X
            {{ W,-W, W},{1,0,0},{1,1,1}}, {{ W,-W,-W},{1,0,0},{1,1,1}},
            {{ W, W,-W},{1,0,0},{1,1,1}}, {{ W, W, W},{1,0,0},{1,1,1}},
            // -X
            {{-W,-W,-W},{-1,0,0},{1,1,1}}, {{-W,-W, W},{-1,0,0},{1,1,1}},
            {{-W, W, W},{-1,0,0},{1,1,1}}, {{-W, W,-W},{-1,0,0},{1,1,1}},
            // +Y
            {{-W, W, W},{0,1,0},{1,1,1}}, {{ W, W, W},{0,1,0},{1,1,1}},
            {{ W, W,-W},{0,1,0},{1,1,1}}, {{-W, W,-W},{0,1,0},{1,1,1}},
            // -Y
            {{-W,-W,-W},{0,-1,0},{1,1,1}}, {{ W,-W,-W},{0,-1,0},{1,1,1}},
            {{ W,-W, W},{0,-1,0},{1,1,1}}, {{-W,-W, W},{0,-1,0},{1,1,1}},
        };
        std::vector<uint32_t> idx;
        for (uint32_t f = 0; f < 6; f++) {
            uint32_t b = f * 4;
            idx.push_back(b+0); idx.push_back(b+1); idx.push_back(b+2);
            idx.push_back(b+0); idx.push_back(b+2); idx.push_back(b+3);
        }

        auto upload = [&](const void* data, VkDeviceSize size,
                          VkBufferUsageFlags usage,
                          VkBuffer& buf, VkDeviceMemory& mem) {
            VkBuffer stg; VkDeviceMemory stgM;
            VulkanBufferCreate::CreateBuffer(vk, size,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stg, stgM);
            void* ptr; vkMapMemory(vk.device, stgM, 0, size, 0, &ptr);
            memcpy(ptr, data, size); vkUnmapMemory(vk.device, stgM);
            VulkanBufferCreate::CreateBuffer(vk, size,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buf, mem);
            VulkanBufferCreate::CopyBuffer(vk, stg, buf, size);
            vkDestroyBuffer(vk.device, stg, nullptr);
            vkFreeMemory(vk.device, stgM, nullptr);
        };

        upload(verts, sizeof(verts), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
               s_cubeGeo.vertexBuffer, s_cubeGeo.vertexMem);
        upload(idx.data(), sizeof(uint32_t)*idx.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
               s_cubeGeo.indexBuffer, s_cubeGeo.indexMem);
        s_cubeGeo.indexCount = (uint32_t)idx.size();
        s_cubeGeo.built = true;
    }

    static void DestroyCubeGeo(VulkanState& vk)
    {
        if (s_cubeGeo.vertexBuffer) { vkDestroyBuffer(vk.device, s_cubeGeo.vertexBuffer, nullptr); vkFreeMemory(vk.device, s_cubeGeo.vertexMem, nullptr); s_cubeGeo.vertexBuffer = VK_NULL_HANDLE; }
        if (s_cubeGeo.indexBuffer)  { vkDestroyBuffer(vk.device, s_cubeGeo.indexBuffer,  nullptr); vkFreeMemory(vk.device, s_cubeGeo.indexMem,  nullptr); s_cubeGeo.indexBuffer  = VK_NULL_HANDLE; }
        s_cubeGeo.built = false;
    }

    static void DestroyInstanceBuffer(VulkanState& vk)
    {
        if (s_instanceBuf.buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(vk.device, s_instanceBuf.buffer, nullptr);
            vkFreeMemory(vk.device, s_instanceBuf.memory, nullptr);
            s_instanceBuf.buffer = VK_NULL_HANDLE;
            s_instanceBuf.valid  = false;
            s_instanceBuf.count  = 0;
        }
    }

    static void BuildDummyInstanceBuffer(VulkanState& vk)
    {
        if (s_dummyInstance.valid) return;

        // 20 floats: mat4(identity) + vec3(0,0,0) + float(0)
        // mat4 identity: [1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1]
        // instanceM[3][3] = 1.0 → шейдер знает что это "не instanced" и берёт pc.model
        float data[20] = {};
        data[0]  = 1.f; // col0.x
        data[5]  = 1.f; // col1.y
        data[10] = 1.f; // col2.z
        data[15] = 1.f; // col3.w  ← instanceM[3][3], проверяется шейдером

        VkDeviceSize size = sizeof(data);
        VulkanBufferCreate::CreateBuffer(vk, size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            s_dummyInstance.buffer, s_dummyInstance.memory);
        void* ptr;
        vkMapMemory(vk.device, s_dummyInstance.memory, 0, size, 0, &ptr);
        memcpy(ptr, data, size);
        vkUnmapMemory(vk.device, s_dummyInstance.memory);
        s_dummyInstance.valid = true;
    }

    static void DestroyDummyInstanceBuffer(VulkanState& vk)
    {
        if (s_dummyInstance.buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(vk.device, s_dummyInstance.buffer, nullptr);
            vkFreeMemory(vk.device, s_dummyInstance.memory, nullptr);
            s_dummyInstance.buffer = VK_NULL_HANDLE;
            s_dummyInstance.valid  = false;
        }
    }

    static void UploadInstanceBuffer(VulkanState& vk, const std::vector<float>& data, uint32_t count)
    {
        DestroyInstanceBuffer(vk);
        if (data.empty() || count == 0) return;
        VkDeviceSize size = sizeof(float) * data.size();
        // HOST_VISIBLE — instance data меняется часто, staging не нужен
        VulkanBufferCreate::CreateBuffer(vk, size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            s_instanceBuf.buffer, s_instanceBuf.memory);
        void* ptr; vkMapMemory(vk.device, s_instanceBuf.memory, 0, size, 0, &ptr);
        memcpy(ptr, data.data(), size);
        vkUnmapMemory(vk.device, s_instanceBuf.memory);
        s_instanceBuf.count = count;
        s_instanceBuf.valid = true;
    }

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
        // ── Instanced path ────────────────────────────────────────────────
        if (mesh.instanceDirty)
        {
            mesh.instanceDirty = false;
            BuildUnitCubeGeo(vk);
            UploadInstanceBuffer(vk, mesh.instanceData, mesh.instanceCount);
        }

        // ── Legacy path (произвольный меш) ────────────────────────────────
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

        // Обновляем фрустум в SceneState — плагины используют для CPU-куллинга
        {
            Camera cam;
            cam.position  = pos;
            cam.target    = pos + front;
            cam.up        = glm::vec3(0,1,0);
            cam.fovDeg    = 90.0f;
            cam.aspect    = static_cast<float>(vk.scExtent.width) /
                            static_cast<float>(vk.scExtent.height);
            cam.nearPlane = nearPlane;
            cam.farPlane  = 500.0f;

            Frustum fr = FrustumOps::BuildFromCamera(cam);
            auto& sc = GetSceneState();
            sc.frustumPlanes = fr.planes;
            sc.frustumReady  = true;
        }
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

        // Dummy instance буфер — биндим на binding 1 для всех non-instanced draws
        // чтобы GPU не читал мусор из непривязанного буфера.
        BuildDummyInstanceBuffer(vk);
        {
            VkBuffer     dummyBufs[] = { s_dummyInstance.buffer };
            VkDeviceSize dummyOffs[] = { 0 };
            vkCmdBindVertexBuffers(cmd, 1, 1, dummyBufs, dummyOffs);
        }

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

        // ── Instanced кубы ──────────────────────────────────────────────────
        // Binding 0 = unit cube geometry (shared), Binding 1 = per-instance data
        if (s_instanceBuf.valid && s_instanceBuf.count > 0 && s_cubeGeo.built)
        {
            // identity push constant — трансформы уже в instance data
            glm::mat4 identity(1.0f);
            vkCmdPushConstants(cmd, vk.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                               0, sizeof(glm::mat4), &identity[0][0]);

            VkBuffer vbufs[]      = { s_cubeGeo.vertexBuffer, s_instanceBuf.buffer };
            VkDeviceSize offs[]   = { 0, 0 };
            vkCmdBindVertexBuffers(cmd, 0, 2, vbufs, offs);
            vkCmdBindIndexBuffer(cmd, s_cubeGeo.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(cmd, s_cubeGeo.indexCount, s_instanceBuf.count, 0, 0, 0);
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
        if (scene.sceneMesh.dirty || scene.sceneMesh.instanceDirty)
        {
            vkWaitForFences(vk.device,
                            static_cast<uint32_t>(vk.inFlightFences.size()),
                            vk.inFlightFences.data(), VK_TRUE, UINT64_MAX);
            UploadSceneMeshIfDirty(vk, scene.sceneMesh);
        }

        // Берём семафор acquire по acquireIndex (round-robin по числу образов),
        // НЕ по currentFrame. Это гарантирует: к моменту следующего acquire
        // этого же образа его семафор уже освобождён presentation-ом.
        const uint32_t acqIdx = vk.acquireIndex;

        uint32_t imageIndex = 0;
        VkResult result = vkAcquireNextImageKHR(
            vk.device, vk.swapchain, UINT64_MAX,
            vk.imageAvailableSemaphores[acqIdx], VK_NULL_HANDLE, &imageIndex);

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

        VkSemaphore waitSems[]   = { vk.imageAvailableSemaphores[acqIdx] };
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

        vk.currentFrame  = (frame + 1) % VulkanState::MAX_FRAMES_IN_FLIGHT;
        vk.acquireIndex  = (acqIdx + 1) % static_cast<uint32_t>(vk.imageAvailableSemaphores.size());
    }

    void DestroyWallBuffers(VulkanState& vk)
    {
        vkDeviceWaitIdle(vk.device);
        VoxelWallBuffer::DestroyAll(vk, s_wallBuffers);
        DestroySceneMeshBuffers(vk);
        DestroyInstanceBuffer(vk);
        DestroyCubeGeo(vk);
        DestroyDummyInstanceBuffer(vk);
    }
}
