// OpenCarWorld.cpp — точка входа DLL-сцены.
// Простой пикап: box-кузов + VehicleConstraint + процедурный мир.
// Без вокселей, без NPC, без грязи — чисто машина и езда.

#include "IScenePlugin.h"
#include "EngineAPI.h"
#include "JoltBridge.h"
#include "SceneState.h"
#include "PhysicsState.h"
#include "Logger.h"

#ifdef RK_JOLT_ENABLED
#include <Jolt/RegisterTypes.h>
#endif

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
            RKeng::Logger::Info("PickupScene: OnLoad");

#ifdef RK_JOLT_ENABLED
            RKeng::InitJoltFromEngine(api);
            JPH::RegisterTypes();
#endif

            // Мир
            RKeng::WorldGen::WorldConfig wCfg;
            wCfg.worldHalfSize = 400.0f;
            wCfg.numRocks      = 50;
            wCfg.numRamps      = 10;
            wCfg.seed          = 1337;
            RKeng::WorldGen::Generate(scene, ph, api, wCfg);

            // Машина
            RKeng::Vec3 spawnPos { 0.0f, 2.0f, 0.0f };
            RKeng::CarLoad::Run(m_car, ph, spawnPos, api);
            RKeng::CarConstraint::Register(m_car, ph);
            RKeng::CarMesh::Build(m_car);

            // Первый кадр камеры
            SyncCamera(scene);

            // Меш в сцену
            SyncMesh(scene);

            RKeng::Logger::Info("PickupScene: ready");
        }

        void OnTick(RKeng::SceneState& scene,
                    RKeng::PhysicsState& ph,
                    float dt) override
        {
            RKeng::CarInputPoll::Run(m_car, scene, dt);
            RKeng::CarTick::Run(m_car, ph, scene, dt);
            SyncMesh(scene);
        }

        void OnUnload(RKeng::SceneState& scene,
                      RKeng::PhysicsState& ph) override
        {
            RKeng::Logger::Info("PickupScene: OnUnload");
            RKeng::CarConstraint::Unregister(m_car, ph);
            RKeng::CarLoad::Destroy(m_car, ph);
            scene.sceneMesh.vertices.clear();
            scene.sceneMesh.indices.clear();
            scene.sceneMesh.dirty = true;
        }

    private:
        void SyncCamera(RKeng::SceneState& scene)
        {
            // Дефолтная позиция камеры до первого тика
            scene.player.worldPos.world.x = m_car.position.x;
            scene.player.worldPos.world.y = m_car.position.y + 5.0f;
            scene.player.worldPos.world.z = m_car.position.z - 10.0f;
            scene.input.yaw   = 0.0f;
            scene.input.pitch = -15.0f;
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

        RKeng::CarState m_car;
    };
}

extern "C"
{
    RK_EXPORT RKeng::IScenePlugin* RK_CreateScene()               { return new PickupScene(); }
    RK_EXPORT void                 RK_DestroyScene(RKeng::IScenePlugin* p) { delete p; }
}
