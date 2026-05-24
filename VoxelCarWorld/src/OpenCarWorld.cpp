// CarScene.cpp
//
// Сцена с управляемой машиной на процедурно сгенерированном мире.
//
// Архитектура:
//   - OnLoad:   инициализация Jolt, генерация мира, спавн машины
//   - OnTick:   ввод → физика машины → обновление меша → синхронизация SceneMesh
//   - OnUnload: освобождение физического тела машины
//
// Правило: никаких прямых инклюдов движковых внутренностей.
// Только sdk/ — движок сам разберётся.

#include "IScenePlugin.h"
#include "EngineAPI.h"
#include "JoltBridge.h"
#include "SceneState.h"
#include "PhysicsState.h"
#include "CarState.h"
#include "WorldGen.h"
#include "CarLoad.h"
#include "CarTick.h"
#include "CarInputPoll.h"
#include "CarMesh.h"
#include "Logger.h"
#include "scene/CarConstraint.h"
#ifdef RK_JOLT_ENABLED
#include <Jolt/RegisterTypes.h>
#endif

namespace
{
    // -----------------------------------------------------------------------
    //  CarScene — полноценная сцена с одной управляемой машиной
    // -----------------------------------------------------------------------
    class CarScene final : public RKeng::IScenePlugin
    {
    public:
        // -------------------------------------------------------------------
        void OnLoad(RKeng::SceneState&  scene,
                    RKeng::PhysicsState& ph,
                    const RKeng::EngineAPI& api) override
        {
            RKeng::Logger::Info("CarScene: OnLoad — начало");

            // ── ШАГ 1: Jolt в этой DLL ─────────────────────────────────────
            // Без этого JPH::Allocate == nullptr → access violation 0xc0000005
            // при первом BoxShapeSettings::Create() внутри WorldGen::Generate().
#ifdef RK_JOLT_ENABLED
            RKeng::InitJoltFromEngine(api);
            JPH::RegisterTypes();   // идемпотентен, Factory уже инициализирована движком
#endif
            RKeng::Logger::Info("CarScene: Jolt инициализирован");

            // ── ШАГ 2: Генерация мира ───────────────────────────────────────
            // WorldGen создаёт пол, барьеры, воксельные стены, трамплины.
            // Конфиг настроен под гоночный полигон: просторный, много трамплинов.
            RKeng::WorldGen::WorldConfig worldCfg;
            worldCfg.worldHalfSize  = 250.0f;
            worldCfg.numVoxelWalls  = 20;    // меньше стен — удобнее ездить
            worldCfg.numRocks       = 15;    // бетонные блоки-препятствия
            worldCfg.numRamps       = 12;    // побольше трамплинов
            worldCfg.seed           = 1337;

            // ВАЖНО: передаём api — WorldGen спавнит тела через него
            m_worldData = RKeng::WorldGen::Generate(scene, ph, api, worldCfg);
            RKeng::Logger::Info("CarScene: мир сгенерирован");

            // ── ШАГ 3: Спавн машины ─────────────────────────────────────────
            // Машина стартует чуть над полом по центру карты.
            // CarLoad::Run создаёт физическое тело + VehicleConstraint.
            RKeng::Vec3 spawnPos { 0.0f, 1.5f, 0.0f };
            RKeng::CarLoad::Run(m_car, ph, spawnPos, &api);

            // ── ШАГ 4: Регистрация VehicleConstraint в PhysicsSystem ────────
            // CarLoad::Run создаёт constraint, но НЕ регистрирует его.
            // Без этого шага Jolt не будет тикать физику машины.
            RKeng::CarConstraint::Register(m_car, ph);

            // Регистрируем коллбек для детекции ударов (урон по вокселям).
            RKeng::CarTick::RegisterContactCallback(m_car, ph);

            // Первичный билд меша — чтобы не ехать с пустым буфером.
            RKeng::CarMesh::Rebuild(m_car);

            // Копируем геометрию в SceneMesh, чтобы движок отрендерил машину
            // уже в первый кадр.
            SyncMeshToScene(scene);

            // Камера для первого кадра — чтобы не было серого экрана до первого тика
            {
                RKeng::Vec3 camOffset = m_car.orientation * m_car.camLocalOffset;
                RKeng::Vec3 camPos    = m_car.position + camOffset;
                scene.player.worldPos.world.x = camPos.x;
                scene.player.worldPos.world.y = camPos.y;
                scene.player.worldPos.world.z = camPos.z;
                scene.input.yaw   = m_car.camYaw;
                scene.input.pitch = m_car.camPitch;
            }

            RKeng::Logger::Info("CarScene: машина заспавнена");
            RKeng::Logger::Info("CarScene: OnLoad — готово");
        }

        // -------------------------------------------------------------------
        void OnTick(RKeng::SceneState&  scene,
                    RKeng::PhysicsState& ph,
                    float dt) override
        {
            // 1. Читаем ввод (W/A/S/D + Shift) → CarInput
            RKeng::CarInputPoll::Run(m_car, scene, dt);

            // 2. Обновляем физику машины: двигатель, подвеска, урон, дебрис
            RKeng::CarTick::Run(m_car, ph, scene, dt, m_worldData.mudZones);

            // 3. Перестраиваем меш, если вокселы изменились (удар, взрыв)
            if (m_car.meshDirty)
            {
                RKeng::CarMesh::Rebuild(m_car);
                m_car.meshDirty = false;
            }

            // 4. Синхронизируем вершины и трансформ в SceneMesh — движок рендерит
            SyncMeshToScene(scene);
        }

        // -------------------------------------------------------------------
        void OnUnload(RKeng::SceneState& scene,
                      RKeng::PhysicsState& ph) override
        {
            RKeng::Logger::Info("CarScene: OnUnload");

            // Сначала убираем VehicleConstraint из PhysicsSystem,
            // затем CarLoad::Destroy вызывает Release() и удаляет тело.
            RKeng::CarConstraint::Unregister(m_car, ph);

            // Очищаем физическое тело и VehicleConstraint
            RKeng::CarLoad::Destroy(m_car, ph);

            // Удаляем все статические тела мира (пол, стены, блоки, трамплины)
            RKeng::WorldGen::Destroy(scene, ph);

            // Убираем геометрию из SceneMesh
            scene.sceneMesh.vertices.clear();
            scene.sceneMesh.indices.clear();
            scene.sceneMesh.dirty = true;
        }

        // -------------------------------------------------------------------
        const char* GetName() const override { return "CarScene"; }

    private:
        // -------------------------------------------------------------------
        //  Переносим текущий меш машины в scene.sceneMesh.
        //  Движок рендерит sceneMesh слепо — формат: pos(3)+color(3)+normal(3).
        // -------------------------------------------------------------------
        void SyncMeshToScene(RKeng::SceneState& scene)
        {
            auto& sm = scene.sceneMesh;

            sm.vertices = m_car.meshVertices;
            sm.indices  = m_car.meshIndices;

            // Трансформ кузова — движок применяет как push constant.
            // Используем позицию + ориентацию из CarState (обновляет CarTick).
            sm.modelMatrix = glm::translate(RKeng::Mat4(1.0f), m_car.position)
                           * glm::mat4_cast(m_car.orientation);

            sm.dirty = true;
        }

        // ── Состояние машины (единственный экземпляр на сцену) ──────────────
        RKeng::CarState m_car;

        // ── Данные мира — нужны CarTick для зон грязи ───────────────────────
        RKeng::WorldGen::WorldData m_worldData;
    };

} // anonymous namespace

// ── Фабрика DLL-плагина ──────────────────────────────────────────────────────
extern "C"
{
    RK_EXPORT RKeng::IScenePlugin* RK_CreateScene()       { return new CarScene(); }
    RK_EXPORT void                 RK_DestroyScene(RKeng::IScenePlugin* p) { delete p; }
}
