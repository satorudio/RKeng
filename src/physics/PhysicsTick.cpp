#include "PhysicsTick.h"
#include "../core/SceneState.h"
#include "../utils/Logger.h"

#ifdef RK_JOLT_ENABLED
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

// Фильтры коллизий для CharacterVirtual::ExtendedUpdate
struct AllBPFilter final : JPH::BroadPhaseLayerFilter {
    bool ShouldCollide(JPH::BroadPhaseLayer) const override { return true; }
};
struct AllLayerFilter final : JPH::ObjectLayerFilter {
    bool ShouldCollide(JPH::ObjectLayer) const override { return true; }
};
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace RKeng::PhysicsTick
{
#ifdef RK_JOLT_ENABLED
    static void TryDestroyVoxelBody(SceneState& scene, PhysicsState& ph,
                                     JPH::BodyID hitBody,
                                     Vec3 impactImpulse = {0,0,0})
    {
        for (auto& wall : scene.voxelWalls)
        {
            for (int c = 0; c < VOXEL_COLS; c++)
            {
                for (int r = 0; r < VOXEL_ROWS; r++)
                {
                    int idx = c * VOXEL_ROWS + r;
                    if (wall.voxelBodyIDs[idx] == hitBody)
                    {
                        ph.bodyInterface->RemoveBody(hitBody);
                        ph.bodyInterface->DestroyBody(hitBody);
                        wall.voxelBodyIDs[idx] = JPH::BodyID();
                        wall.DestroyVoxel(c, r, impactImpulse);
                        Logger::Info("Voxel destroyed (collision): wall=" +
                            std::to_string(wall.id) +
                            " col=" + std::to_string(c) +
                            " row=" + std::to_string(r));
                        return;
                    }
                }
            }
        }
    }

    static void CheckPlayerVoxelCollision(SceneState& scene, PhysicsState& ph)
    {
        if (!ph.character) return;

        JPH::RVec3 charPos = ph.character->GetPosition();
        Vec3 playerPos(
            static_cast<float>(charPos.GetX()),
            static_cast<float>(charPos.GetY()),
            static_cast<float>(charPos.GetZ()));

        const float playerR = scene.player.radius + 0.05f;
        const float playerH = scene.player.currentHeight;
        const float H       = VOXEL_SIZE * 0.5f;

        float pMinX = playerPos.x - playerR, pMaxX = playerPos.x + playerR;
        float pMinY = playerPos.y,           pMaxY = playerPos.y + playerH;
        float pMinZ = playerPos.z - playerR, pMaxZ = playerPos.z + playerR;

        JPH::Vec3 vel = ph.character->GetLinearVelocity();
        Vec3 playerVel(vel.GetX(), vel.GetY(), vel.GetZ());
        float speed = sqrtf(playerVel.x*playerVel.x +
                            playerVel.y*playerVel.y +
                            playerVel.z*playerVel.z);
        if (speed < 0.1f) return;

        constexpr int MAX_DESTROY_PER_FRAME = 3;
        int destroyedThisFrame = 0;

        for (auto& wall : scene.voxelWalls)
        {
            for (int c = 0; c < VOXEL_COLS; c++)
            {
                for (int r = 0; r < VOXEL_ROWS; r++)
                {
                    // BUG FIX: было `return` — выходило из всей функции, пропуская
                    // оставшиеся стены. Теперь `goto done` — выходим только из циклов.
                    if (destroyedThisFrame >= MAX_DESTROY_PER_FRAME) goto done;
                    if (!wall.alive[c][r]) continue;

                    Vec3 vp = wall.VoxelWorldPos(c, r);
                    bool overlap =
                        pMaxX > vp.x - H && pMinX < vp.x + H &&
                        pMaxY > vp.y - H && pMinY < vp.y + H &&
                        pMaxZ > vp.z - H && pMinZ < vp.z + H;
                    if (!overlap) continue;

                    Vec3 toVoxel { vp.x - playerPos.x, 0.0f, vp.z - playerPos.z };
                    float tLen = sqrtf(toVoxel.x*toVoxel.x + toVoxel.z*toVoxel.z);
                    if (tLen > 0.001f) { toVoxel.x /= tLen; toVoxel.z /= tLen; }
                    float impStr = std::min(speed * 0.8f, 6.0f);
                    Vec3 impulse{ toVoxel.x * impStr, 1.5f, toVoxel.z * impStr };

                    int idx = c * VOXEL_ROWS + r;
                    JPH::BodyID bid = wall.voxelBodyIDs[idx];
                    if (!bid.IsInvalid())
                        TryDestroyVoxelBody(scene, ph, bid, impulse);
                    else
                        wall.DestroyVoxel(c, r, impulse);
                    ++destroyedThisFrame;
                }
            }
        }
        done:;
    }
#endif

    static void UpdateFallingVoxels(SceneState& scene, float dt)
    {
        for (auto& wall : scene.voxelWalls)
            if (!wall.fallingVoxels.empty())
                wall.UpdateFalling(dt);
    }

    void Run(PhysicsState& ph, float deltaTime)
    {
        auto& scene = GetSceneState();

        Logger::Trace("  PT: UpdateFallingVoxels start");
        UpdateFallingVoxels(scene, deltaTime);
        Logger::Trace("  PT: UpdateFallingVoxels done");

#ifdef RK_JOLT_ENABLED
        if (!ph.initialized) {
            Logger::Warn("  PT: physics not initialized, skip");
            return;
        }

        Logger::Trace("  PT: sanity checks...");
        if (!ph.physicsSystem) { Logger::Fatal("  PT: physicsSystem is NULL!"); return; }
        if (!ph.tempAllocator) { Logger::Fatal("  PT: tempAllocator is NULL!"); return; }
        if (!ph.jobSystem)     { Logger::Fatal("  PT: jobSystem is NULL!"); return; }
        if (!ph.bodyInterface) { Logger::Fatal("  PT: bodyInterface is NULL!"); return; }
        Logger::Trace("  PT: sanity OK");

        {
            char buf[128];
            snprintf(buf, sizeof(buf),
                "  PT: accumulator += %.6f  acc=%.6f  step=%.6f",
                deltaTime, ph.accumulator, ph.fixedTimestep);
            Logger::Trace(buf);
        }
        ph.accumulator += deltaTime;

        int stepIdx = 0;
        while (ph.accumulator >= ph.fixedTimestep)
        {
            {
                char buf[64];
                snprintf(buf, sizeof(buf),
                    "  PT: physicsSystem->Update [step %d]", stepIdx);
                Logger::Trace(buf);
            }

            if (ph.character) {
                auto p = ph.character->GetPosition();
                char buf[128];
                snprintf(buf, sizeof(buf), "    char pos=%.3f %.3f %.3f",
                    p.GetX(), p.GetY(), p.GetZ());
                Logger::Trace(buf);
            } else {
                Logger::Trace("    character is NULL (no player scene)");
            }

            {
                char buf[128];
                snprintf(buf, sizeof(buf),
                    "    numBodies=%u  numActiveBodies=%u",
                    ph.physicsSystem->GetNumBodies(),
                    ph.physicsSystem->GetNumActiveBodies(JPH::EBodyType::RigidBody));
                Logger::Trace(buf);
            }

            ph.physicsSystem->Update(
                ph.fixedTimestep,
                ph.collisionSteps,
                ph.tempAllocator.get(),
                ph.jobSystem.get());

            // CharacterVirtual::ExtendedUpdate — сразу после physicsSystem->Update
            if (ph.character)
            {
                auto& player = scene.player;
                auto* ch = ph.character.get();

                ch->UpdateGroundVelocity();
                player.onGround = (ch->GetGroundState() ==
                                   JPH::CharacterVirtual::EGroundState::OnGround);

                JPH::CharacterVirtual::ExtendedUpdateSettings updateSettings;
                AllBPFilter    bpFilter;
                AllLayerFilter layerFilter;
                ch->ExtendedUpdate(
                    ph.fixedTimestep,
                    ph.physicsSystem->GetGravity(),
                    updateSettings,
                    bpFilter,
                    layerFilter,
                    {},
                    {},
                    *ph.tempAllocator);

                JPH::RVec3 jPos = ch->GetPosition();
                player.worldPos.world = DVec3(
                    static_cast<double>(jPos.GetX()),
                    static_cast<double>(jPos.GetY()),
                    static_cast<double>(jPos.GetZ()));
            }

            ph.accumulator -= ph.fixedTimestep;
            stepIdx++;
        }

        Logger::Trace("  PT: CheckPlayerVoxelCollision");
        CheckPlayerVoxelCollision(scene, ph);

        Logger::Trace("  PT: all done");
#else
        (void)ph;
#endif
    }
}
