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

#include <sdk/IScenePlugin.h>
#include <sdk/EngineAPI.h>
#include <sdk/JoltBridge.h>
#include <sdk/SceneState.h>
#include <sdk/PhysicsState.h>
#include <sdk/CarState.h>
#include <sdk/WorldGen.h>
#include <sdk/CarLoad.h>
#include <sdk/CarTick.h>
#include <sdk/CarInputPoll.h>
#include <sdk/CarMesh.h>
#include <sdk/Logger.h>

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
            worldCfg.worldSize      = 250.0f;
            worldCfg.numVoxelWalls  = 20;    // меньше стен — удобнее ездить
            worldCfg.numSolidBlocks = 15;    // бетонные блоки-препятствия
            worldCfg.numRamps       = 12;    // побольше трамплинов
            worldCfg.seed           = 1337;

            RKeng::WorldGen::Generate(scene, ph, worldCfg);
            RKeng::Logger::Info("CarScene: мир сгенерирован");

            // ── ШАГ 3: Спавн машины ─────────────────────────────────────────
            // Машина стартует чуть над полом по центру карты.
            // CarLoad::Run создаёт физическое тело + VehicleConstraint.
            RKeng::Vec3 spawnPos { 0.0f, 1.5f, 0.0f };
            RKeng::CarLoad::Run(m_car, ph, spawnPos);

            // Регистрируем коллбек для детекции ударов (урон по вокселям).
            RKeng::CarTick::RegisterContactCallback(m_car, ph);

            // Первичный билд меша — чтобы не ехать с пустым буфером.
            RKeng::CarMesh::Rebuild(m_car);

            // Копируем геометрию в SceneMesh, чтобы движок отрендерил машину
            // уже в первый кадр.
            SyncMeshToScene(scene);

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
            RKeng::CarTick::Run(m_car, ph, scene, dt);

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

            // Очищаем физическое тело и VehicleConstraint
            RKeng::CarLoad::Destroy(m_car, ph);

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
    };

} // anonymous namespace

// ── Фабрика DLL-плагина ──────────────────────────────────────────────────────
extern "C"
{
    RK_EXPORT RKeng::IScenePlugin* RK_CreateScene()       { return new CarScene(); }
    RK_EXPORT void                 RK_DestroyScene(RKeng::IScenePlugin* p) { delete p; }
}
