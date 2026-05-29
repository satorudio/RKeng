// OpenCarWorld.cpp — точка входа DLL-сцены.
// Машина создаётся через api.SpawnVehicle() — никакого Jolt в DLL.

#include "IScenePlugin.h"
#include "EngineAPI.h"
#include "SceneState.h"
#include "PhysicsState.h"
#include "Logger.h"

#include "scene/CarState.h"
#include "scene/CarLoad.h"
#include "scene/CarMesh.h"
#include "scene/CarTick.h"
#include "scene/CarInputPoll.h"
#include "scene/CarConstraint.h"
#include "scene/WorldGen.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace
{
    class PickupScene final : public RKeng::IScenePlugin
    {
    public:
        const char* GetName() const override { return "PickupScene"; }

        void OnLoad(RKeng::SceneState& scene,
                    RKeng::PhysicsState& ph,
                    const RKeng::EngineAPI& api) override
        {
            m_api = api; if (api.LogInfo) { auto s = "ph.initialized=" + std::to_string(ph.initialized); api.LogInfo(s.c_str()); }
            RKeng::Logger::Info("PickupScene: OnLoad");

            if (api.engineVersion < 6) {
                if (api.LogError) api.LogError("PickupScene: engine v6+ required (SpawnVehicle)");
                return;
            }

            // Мир
            RKeng::WorldGen::WorldConfig wCfg;
            wCfg.worldHalfSize = 400.0f;
            wCfg.numRocks      = 50;
            wCfg.numRamps      = 10;
            wCfg.seed          = 1337;
            RKeng::WorldGen::Generate(scene, ph, api, wCfg);

            // ОБЯЗАТЕЛЬНО после всех статических тел — иначе колёса проваливаются сквозь пол
            if (api.OptimizeBroadPhase) api.OptimizeBroadPhase(ph);

            // Машина — VehicleConstraint создаётся внутри движка
            RKeng::Vec3 spawnPos { 0.0f, 2.0f, 0.0f };
            RKeng::CarLoad::Run(m_car, ph, spawnPos, api);

            if (!m_car.initialized) {
                if (api.LogError) api.LogError("PickupScene: CarLoad failed");
                return;
            }

            // CarConstraint::Register — no-op (SpawnVehicle уже добавил constraint)
            RKeng::CarConstraint::Register(m_car, ph);

            RKeng::CarMesh::Build(m_car); m_car.camPitch = 0.0f; m_car.camYaw = 0.0f;

            scene.thirdPersonCamera = true;   // движок не добавляет currentHeight*0.85 — позицию мы выставляем сами в CarTick
            scene.player.currentHeight = 0.0f;
            SyncCamera(scene);
            SyncMesh(scene);

            RKeng::Logger::Info("PickupScene: ready");
        }

        void OnTick(RKeng::SceneState& scene,
                    RKeng::PhysicsState& ph,
                    float dt) override
        {
            RKeng::CarInputPoll::Run(m_car, scene, dt);
            RKeng::CarTick::Run(m_car, ph, scene, dt, m_api);
            SyncMesh(scene);
        }

        void OnUnload(RKeng::SceneState& scene,
                      RKeng::PhysicsState& ph) override
        {
            RKeng::Logger::Info("PickupScene: OnUnload");
            scene.thirdPersonCamera = false;
            RKeng::CarConstraint::Unregister(m_car, ph);
            RKeng::CarLoad::Destroy(m_car, ph, m_api);
            scene.sceneMesh.vertices.clear();
            scene.sceneMesh.indices.clear();
            scene.sceneMesh.dirty = true;
        }

    private:
        void SyncCamera(RKeng::SceneState& scene)
        {
            // Первоначальная позиция — над машиной (до первого тика CarTick)
            scene.player.worldPos.world.x = m_car.position.x;
            scene.player.worldPos.world.y = m_car.position.y + m_car.params.halfH * 1.6f;
            scene.player.worldPos.world.z = m_car.position.z + m_car.params.halfL * 0.5f;
            scene.input.yaw   = 0.0f;
            scene.input.pitch = 0.0f;
        }

        void SyncMesh(RKeng::SceneState& scene)
        {
            auto& sm = scene.sceneMesh;
            sm.vertices    = m_car.meshVertices;
            sm.indices     = m_car.meshIndices;
            sm.modelMatrix = glm::translate(RKeng::Mat4(1.0f), m_car.position)
                           * glm::mat4_cast(m_car.orientation);
            sm.dirty = true;
        }

        RKeng::CarState  m_car;
        RKeng::EngineAPI m_api;
    };
}

extern "C"
{
    RK_EXPORT RKeng::IScenePlugin* RK_CreateScene()                    { return new PickupScene(); }
    RK_EXPORT void                 RK_DestroyScene(RKeng::IScenePlugin* p) { delete p; }
}
