// SceneTemplate.cpp — шаблон сцены.
// Переименуй: SceneTemplate → MySceneName (везде в этом файле)
// Поменяй SCENE_NAME на строку которую будешь передавать в SceneLoad.
//
// Как добавить новую сцену:
//   1. cp SceneTemplate.h MyScene.h && cp SceneTemplate.cpp MyScene.cpp
//   2. Поменяй namespace и SCENE_NAME
//   3. cmake .. && ninja   (cmake нужен один раз чтобы GLOB подхватил файлы)
//   4. В EngineInit.cpp или конфиге поменяй имя активной сцены на SCENE_NAME
//      — рекомпилировать движок больше не нужно при последующих добавлениях сцен.

#include "SceneTemplate.h"
#include "../core/SceneRegistry.h"

namespace RKeng::SceneTemplate
{
    static constexpr const char* SCENE_NAME = "template";

    void Load(SceneState& scene, PhysicsState& physics)
    {
        // TODO: настрой сцену здесь:
        //   - размести тела в physics
        //   - заполни scene.voxelWalls, scene.player spawn и т.д.
        (void)scene; (void)physics;
    }

    // Авторегистрация при старте программы — никаких изменений в движке не нужно.
    static const bool s_registered = SceneRegistry::Register(SCENE_NAME, Load);
}
