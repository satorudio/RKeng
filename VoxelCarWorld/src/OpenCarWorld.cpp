// CarScene.cpp — сцена с управляемой машиной на процедурно сгенерированном мире.
//
// Порядок include ВАЖЕН:
//   1. scene/CarState.h — расширенная версия стейта машины для этой DLL.
//      Должна идти ДО любых sdk/ заголовков которые тянут sdk/CarState.h.
//   2. Далее sdk/ заголовки.
//   3. scene/ заголовки сцены.

// ── 1. Расширенный стейт машины — ПЕРВЫМ ────────────────────────────────────
#include "scene/CarState.h"

// ── 2. SDK-заголовки ─────────────────────────────────────────────────────────
#include "IScenePlugin.h"
#include "EngineAPI.h"
#include "JoltBridge.h"
#include "SceneState.h"
#include "PhysicsState.h"
#include "Logger.h"
#ifdef RK_JOLT_ENABLED
#include <Jolt/RegisterTypes.h>
#endif

// ── 3. DLL-специфичные заголовки сцены ───────────────────────────────────────
#include "scene/WorldGen.h"
#include "scene/CarLoad.h"
#include "scene/CarTick.h"
#include "scene/CarInputPoll.h"
#include "scene/CarMesh.h"
#include "scene/CarConstraint.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace
{
    class CarScene final : public RKeng::IScenePlugin
    {
    public:
        void OnLoad(RKeng::SceneState&   scene,
                    RKeng::PhysicsState& ph,
                    const RKeng::EngineAPI& api) override
        {
            RKeng::Logger::Info("CarScene: OnLoad — начало");

            // ── ШАГ 1: Jolt синглтоны ──────────────────────────────────────
#ifdef RK_JOLT_ENABLED
            RKeng::InitJoltFromEngine(api);
            JPH::RegisterTypes();
#endif
            RKeng::Logger::Info("CarScene: Jolt инициализирован");

            // ── ШАГ 2: Генерация мира ──────────────────────────────────────
            RKeng::WorldGen::WorldConfig worldCfg;
            worldCfg.worldHalfSize  = 250.0f;
            worldCfg.numVoxelWalls  = 20;
            worldCfg.numRocks       = 15;
            worldCfg.numRamps       = 12;
            worldCfg.seed           = 1337;

            m_worldData = RKeng::WorldGen::Generate(scene, ph, api, worldCfg);
            RKeng::Logger::Info("CarScene: мир сгенерирован");

            // ── ШАГ 3: Спавн машины ────────────────────────────────────────
            RKeng::Vec3 spawnPos { 0.0f, 1.5f, 0.0f };
            RKeng::CarLoad::Run(m_car, ph, spawnPos, &api);

            // ── ШАГ 4: Регистрация VehicleConstraint в PhysicsSystem ────────
            RKeng::CarConstraint::Register(m_car, ph);

            // ── ШАГ 5: Contact callback, первичный меш, камера ──────────────
            RKeng::CarTick::RegisterContactCallback(m_car, ph);
            RKeng::CarMesh::Rebuild(m_car);
            SyncMeshToScene(scene);

            {
                RKeng::Vec3 camOffset = m_car.orientation * m_car.camLocalOffset;
                RKeng::Vec3 camPos    = m_car.position + camOffset;
                scene.player.worldPos.world.x = camPos.x;
                scene.player.worldPos.world.y = camPos.y;
                scene.player.worldPos.world.z = camPos.z;
                scene.input.yaw   = m_car.camYaw;
                scene.input.pitch = m_car.camPitch;
            }

            RKeng::Logger::Info("CarScene: OnLoad — готово");
        }

        void OnTick(RKeng::SceneState&   scene,
                    RKeng::PhysicsState& ph,
                    float dt) override
        {
            RKeng::CarInputPoll::Run(m_car, scene, dt);
            RKeng::CarTick::Run(m_car, ph, scene, dt, m_worldData.mudZones);

            if (m_car.meshDirty)
            {
                RKeng::CarMesh::Rebuild(m_car);
                m_car.meshDirty = false;
            }

            SyncMeshToScene(scene);
        }

        void OnUnload(RKeng::SceneState& scene,
                      RKeng::PhysicsState& ph) override
        {
            RKeng::Logger::Info("CarScene: OnUnload");
            RKeng::CarConstraint::Unregister(m_car, ph);
            RKeng::CarLoad::Destroy(m_car, ph);
            RKeng::WorldGen::Destroy(scene, ph);
            scene.sceneMesh.vertices.clear();
            scene.sceneMesh.indices.clear();
            scene.sceneMesh.dirty = true;
        }

        const char* GetName() const override { return "CarScene"; }

    private:
        void SyncMeshToScene(RKeng::SceneState& scene)
        {
            auto& sm = scene.sceneMesh;
            sm.vertices    = m_car.meshVertices;
            sm.indices     = m_car.meshIndices;
            sm.modelMatrix = glm::translate(RKeng::Mat4(1.0f), m_car.position)
                           * glm::mat4_cast(m_car.orientation);
            sm.dirty = true;
        }

        RKeng::CarState            m_car;
        RKeng::WorldGen::WorldData m_worldData;
    };

} // anonymous namespace

extern "C"
{
    RK_EXPORT RKeng::IScenePlugin* RK_CreateScene()                    { return new CarScene(); }
    RK_EXPORT void                 RK_DestroyScene(RKeng::IScenePlugin* p) { delete p; }
}
