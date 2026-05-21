#pragma once
// IScenePlugin.h — единственный публичный контракт между движком и сценой.
//
// ЭТОТ ФАЙЛ — часть публичного API движка (RKeng SDK).
// Копируй его в проект сцены. Ничего из src/ сцене знать не нужно.
//
// Архитектура:
//   RKengCore.dll  ← движок + Jolt, собирается один раз
//   VoxelWorld.dll ← твоя сцена/игра, реализует IScenePlugin
//   RKeng.exe      ← тонкий лончер, линкуется к RKengCore.dll
//
// Jolt живёт внутри RKengCore.dll — один инстанс на весь процесс.
// Синглтоны (Factory, аллокаторы) общие автоматически для всех
// плагинов, слинкованных с libRKengCore.dll.a. void* в EngineAPI
// не нужны, InitJoltFromEngine() не нужен.
//
// Чтобы сделать новую сцену:
//   1. Создай DLL проект, скопируй sdk/ (IScenePlugin.h + EngineAPI.h)
//   2. Реализуй класс : public IScenePlugin
//   3. Экспортируй фабрику (см. ниже)
//   4. Слинкуй с build/libRKengCore.dll.a
//   5. Скомпилируй отдельно — движок трогать не нужно

#include <cstdint>

namespace RKeng
{
    struct SceneState;
    struct PhysicsState;
    struct EngineAPI;
}

namespace RKeng
{
    class IScenePlugin
    {
    public:
        virtual ~IScenePlugin() = default;

        virtual void OnLoad  (SceneState& scene, PhysicsState& physics, const EngineAPI& api) = 0;
        virtual void OnTick  (SceneState& scene, PhysicsState& physics, float dt) { (void)scene; (void)physics; (void)dt; }
        virtual void OnUnload(SceneState& scene, PhysicsState& physics)           { (void)scene; (void)physics; }

        virtual const char* GetName() const = 0;
    };

    using CreateSceneFn  = IScenePlugin* (*)();
    using DestroySceneFn = void (*)(IScenePlugin*);

} // namespace RKeng

// ── DLL export/import макрос ─────────────────────────────────────────────────
#if defined(_WIN32) || defined(__CYGWIN__)
#  if defined(RK_ENGINE_BUILD) || defined(RK_SCENE_BUILD)
#    ifdef __GNUC__
#      define RK_API    __attribute__((dllexport))
#      define RK_EXPORT __attribute__((dllexport))
#    else
#      define RK_API    __declspec(dllexport)
#      define RK_EXPORT __declspec(dllexport)
#    endif
#  else
#    ifdef __GNUC__
#      define RK_API    __attribute__((dllimport))
#      define RK_EXPORT __attribute__((dllexport))
#    else
#      define RK_API    __declspec(dllimport)
#      define RK_EXPORT __declspec(dllexport)
#    endif
#  endif
#else
#  define RK_API    __attribute__((visibility("default")))
#  define RK_EXPORT __attribute__((visibility("default")))
#endif
