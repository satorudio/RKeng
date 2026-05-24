#pragma once
#include "MathTypes.h"
#include "PhysicsState.h"
#include "VoxelWall.h"
#include <vector>
#include <array>
#include <cstdint>

// SceneState.h — всё состояние текущей сцены.


struct GLFWwindow;  // forward decl — не тащим GLFW заголовок в публичный SDK

#ifdef RK_JOLT_ENABLED
#include <Jolt/Physics/Body/BodyID.h>
#endif

namespace RKeng
{
    struct PlayerState
    {
        WorldPos  worldPos;
        Vec3      velocity       { 0,0,0 };
        Quat      rotation       { 1,0,0,0 };

        float     height         = 1.8f;
        float     radius         = 0.35f;
        float     walkSpeed      = 5.0f;
        float     runSpeed       = 10.0f;
        float     crouchSpeed    = 2.5f;
        float     jumpImpulse    = 8.0f;
        float     crouchHeight   = 0.9f;

        bool      onGround       = false;
        bool      isCrouching    = false;
        bool      isRunning      = false;

        float     currentHeight  = 1.8f;

        bool      isSomersaulting = false;
        float     somersaultAngle = 0.0f;
        bool      canDoubleJump   = false;

#ifdef RK_JOLT_ENABLED
        JPH::BodyID floorBodyID;
#endif
    };

    struct InputState
    {
        bool forward  = false;
        bool backward = false;
        bool left     = false;
        bool right    = false;
        bool jump     = false;
        bool jumpPressed = false;
        bool crouch   = false;
        bool run      = false;

        float mouseDeltaX = 0.0f;
        float mouseDeltaY = 0.0f;
        float yaw         = 0.0f;
        float pitch       = 0.0f;
    };

    // ── Generic меш-слот для DLL-сцены ──────────────────────────────────────
    // Сцена пишет сюда геометрию (машина, объекты), движок рендерит слепо.
    // Формат вершины: pos(3) + color(3) + normal(3) = 9 floats
    // modelMatrix — трансформ (используется как push constant)
    struct SceneMesh
    {
        std::vector<float>    vertices;
        std::vector<uint32_t> indices;
        Mat4                  modelMatrix = Mat4(1.0f);
        bool                  dirty       = false;

        // Instanced rendering: mat4(16)+color(3)+wire(1) = 20 floats per instance
        std::vector<float>    instanceData;
        uint32_t              instanceCount = 0;
        bool                  instanceDirty = false;
    };

    struct SceneState
    {
        PlayerState player;
        InputState  input;

        DVec3 originShift { 0.0, 0.0, 0.0 };

        float deltaTime   = 0.0f;
        float totalTime   = 0.0f;

        // Воксельные разрушаемые стены (рендерятся движком)
        std::vector<VoxelWall> voxelWalls;

        // Обобщённый меш сцены — машина, объекты, etc.
        // DLL-сцена обновляет, движок рендерит.
        SceneMesh sceneMesh;

        // Хэндл окна — движок заполняет после создания GLFW окна.
        // DLL-сцена использует для опроса клавиш (GLFW не требует линковки с движком).
        GLFWwindow* windowHandle = nullptr;

        // Frustum-плоскости текущего кадра (world space, нормаль внутрь).
        // Движок пишет каждый кадр перед OnTick. Плагин использует для куллинга.
        // Layout: [Left, Right, Bottom, Top, Near, Far]
        // Проверка AABB: для каждой плоскости (nx,ny,nz,d) — если positive-vertex < 0 → culled.
        std::array<Vec4, 6> frustumPlanes {};
        bool frustumReady = false;

#ifdef RK_JOLT_ENABLED
        JPH::BodyID floorBodyID;
        JPH::BodyID wallBodyIDs[4];
        std::vector<JPH::BodyID> worldStaticBodyIDs; // все статики от WorldGen
#endif
    };

    SceneState& GetSceneState();
}
