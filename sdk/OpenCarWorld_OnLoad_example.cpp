// OpenCarWorld_OnLoad_example.cpp
//
// Пример правильного OnLoad() для DLL-сцены.
// Корневая причина краша: JPH::Allocate == nullptr в DLL → 0xc0000005
// при первом BoxShapeSettings::Create() внутри WorldGen::Generate().
//
// Решение: вызвать RKeng::InitJoltFromEngine(api) ДО любого кода Jolt.

#include <sdk/IScenePlugin.h>
#include <sdk/EngineAPI.h>
#include <sdk/JoltBridge.h>       // ← RKeng::InitJoltFromEngine()
#include <sdk/SceneState.h>
#include <sdk/PhysicsState.h>
#include <sdk/WorldGen.h>
#include <sdk/SceneLoad.h>
#include <sdk/Logger.h>

#ifdef RK_JOLT_ENABLED
#include <Jolt/RegisterTypes.h>   // JPH::RegisterTypes() — безопасно после InitJoltFromEngine
#endif

namespace
{
    class OpenCarWorld final : public RKeng::IScenePlugin
    {
    public:
        void OnLoad(RKeng::SceneState& scene,
                    RKeng::PhysicsState& ph,
                    const RKeng::EngineAPI& api) override
        {
            RKeng::Logger::Info("OpenCarWorld: OnLoad step 1");

            // ── ШАГ 2: ИНИЦИАЛИЗАЦИЯ JOLT В ЭТОЙ DLL ─────────────────────────
            // ОБЯЗАТЕЛЬНО первым, до любого вызова Jolt.
            // Без этого JPH::Allocate == nullptr → access violation 0xc0000005
            // при первом BoxShapeSettings::Create() в WorldGen::Generate().
#ifdef RK_JOLT_ENABLED
            RKeng::InitJoltFromEngine(api);
            // RegisterTypes безопасно вызывать — идемпотентен при общей Factory.
            // НЕ вызывай RegisterDefaultAllocator() и НЕ создавай new JPH::Factory() —
            // они перезапишут только что прописанные синглтоны движка.
            JPH::RegisterTypes();
#endif
            RKeng::Logger::Info("OpenCarWorld: OnLoad step 2 - InitJolt done");

            // ── ШАГ 3: ГЕНЕРАЦИЯ МИРА ────────────────────────────────────────
            // Теперь Jolt полностью инициализирован в этой DLL — краша не будет.
            RKeng::Logger::Info("OpenCarWorld: step 3 - WorldGen::Generate");
            RKeng::SceneLoad::Run(scene, ph);

            RKeng::Logger::Info("OpenCarWorld: OnLoad done");
        }

        void OnTick(RKeng::SceneState& scene,
                    RKeng::PhysicsState& ph,
                    float dt) override
        {
            (void)scene; (void)ph; (void)dt;
            // TODO: CarTick, PlayerMove, CarInputPoll и т.д.
        }

        void OnUnload(RKeng::SceneState& scene,
                      RKeng::PhysicsState& ph) override
        {
            (void)scene; (void)ph;
            // TODO: cleanup
        }

        const char* GetName() const override { return "OpenCarWorld"; }
    };
}

extern "C"
{
    RK_EXPORT RKeng::IScenePlugin* RK_CreateScene()  { return new OpenCarWorld(); }
    RK_EXPORT void RK_DestroyScene(RKeng::IScenePlugin* p) { delete p; }
}
