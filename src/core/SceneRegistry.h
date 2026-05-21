#pragma once
// SceneRegistry.h — реестр сцен.
// Чтобы добавить новую сцену:
//   1. Создай MyScene.h / MyScene.cpp в src/scene/
//   2. В MyScene.cpp в анонимном namespace зарегистрируй:
//        static const bool _reg = SceneRegistry::Register("my_scene", MyScene::Load);
//   3. Поменяй имя сцены в config или SceneLoad::Run() — рекомпилировать движок не нужно.
// Cmake GLOB_RECURSE подхватит .cpp автоматически, но нужен cmake .. (не ninja).

#include "../physics/PhysicsState.h"
#include "SceneState.h"
#include <functional>
#include <string>
#include <unordered_map>

namespace RKeng::SceneRegistry
{
    using LoadFn = std::function<void(SceneState&, PhysicsState&)>;

    // Регистрирует загрузчик сцены по имени.
    // Возвращает true чтобы можно было писать: static const bool _ = Register(...);
    bool Register(const std::string& name, LoadFn fn);

    // Загружает сцену по имени. Бросает std::runtime_error если имя не найдено.
    void Load(const std::string& name, SceneState& scene, PhysicsState& physics);

    // Возвращает список всех зарегистрированных сцен (для дебага / editor UI).
    std::vector<std::string> ListScenes();
}
