#pragma once
// SceneTemplate.h — шаблон для создания новой сцены.
// Скопируй этот файл + SceneTemplate.cpp, переименуй, и зарегистрируй.
// Больше ничего трогать не нужно — движок найдёт сцену через SceneRegistry.

#include "../core/SceneState.h"
#include "../physics/PhysicsState.h"

namespace RKeng::SceneTemplate
{
    void Load(SceneState& scene, PhysicsState& physics);
}
