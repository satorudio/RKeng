# Написать плагин для RKeng

Вся игровая логика живёт в DLL-плагине — отдельном файле который движок загружает при старте.  
Движок пересобирать не нужно. Плагин компилируется отдельно.

---

## Минимальный плагин

### 1. Скопируй SDK

```
sdk/
  IScenePlugin.h   ← интерфейс плагина
  EngineAPI.h      ← API движка (физика, логгер, etc.)
  SceneState.h     ← состояние сцены (игрок, инпут, меши)
```

### 2. Реализуй IScenePlugin

```cpp
// MyScene.h
#pragma once
#include "sdk/IScenePlugin.h"
#include "sdk/EngineAPI.h"

class MyScene : public RKeng::IScenePlugin {
public:
    const char* GetName() const override { return "my_scene"; }

    void OnLoad(RKeng::SceneState& scene,
                RKeng::PhysicsState& physics,
                const RKeng::EngineAPI& api) override;

    void OnTick(RKeng::SceneState& scene,
                RKeng::PhysicsState& physics,
                float dt) override;

    void OnUnload(RKeng::SceneState& scene,
                  RKeng::PhysicsState& physics) override;
};
```

```cpp
// MyScene.cpp
#include "MyScene.h"

void MyScene::OnLoad(RKeng::SceneState& scene,
                     RKeng::PhysicsState& physics,
                     const RKeng::EngineAPI& api)
{
    api.LogInfo("MyScene загружена");

    // Спавн пола
    RKeng::RK_BoxBody floor{};
    floor.position    = { 0, -1, 0 };
    floor.halfExtents = { 50, 1, 50 };
    api.SpawnStaticBox(physics, floor);
}

void MyScene::OnTick(RKeng::SceneState& scene,
                     RKeng::PhysicsState& physics,
                     float dt)
{
    // Читаем инпут
    if (scene.input.forward) { /* ... */ }
}

void MyScene::OnUnload(RKeng::SceneState& scene,
                       RKeng::PhysicsState& physics)
{
    // Освободить ресурсы
}
```

### 3. Экспортируй фабрику

```cpp
// В конце MyScene.cpp или в отдельном factory.cpp

extern "C" {
    RK_EXPORT RKeng::IScenePlugin* RK_CreateScene() {
        return new MyScene();
    }
    RK_EXPORT void RK_DestroyScene(RKeng::IScenePlugin* p) {
        delete p;
    }
}
```

### 4. CMakeLists.txt плагина

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyScene)

set(CMAKE_CXX_STANDARD 20)

add_library(MyScene SHARED
    MyScene.cpp
)

target_include_directories(MyScene PRIVATE
    ${CMAKE_SOURCE_DIR}/sdk
)

# Линкуем с движком
target_link_libraries(MyScene PRIVATE
    ${PATH_TO_RKENG}/build/libRKengCore.dll.a
)

target_compile_definitions(MyScene PRIVATE RK_SCENE_BUILD)
```

---

## Рендер геометрии

Движок рендерит `scene.sceneMesh` каждый кадр — ты просто пишешь туда вершины.

```cpp
// Формат вершины: pos(3) + color(3) + normal(3) = 9 floats
void MyScene::OnLoad(...) {
    auto& mesh = scene.sceneMesh;
    
    // Треугольник
    mesh.vertices = {
    //  x      y     z     r    g    b    nx   ny   nz
        0.0f,  1.0f, 0.0f, 1.0f,0.0f,0.0f, 0,1,0,
       -1.0f, -1.0f, 0.0f, 0.0f,1.0f,0.0f, 0,1,0,
        1.0f, -1.0f, 0.0f, 0.0f,0.0f,1.0f, 0,1,0,
    };
    mesh.indices = { 0, 1, 2 };
    mesh.dirty = true;  // ← обязательно, иначе движок не перезальёт буфер
}
```

Для инстансинга — заполни `mesh.instanceData` и `mesh.instanceCount`.  
Формат инстанса: `mat4(16) + color(3) + wireframe(1)` = 20 floats.

---

## Инпут

```cpp
void MyScene::OnTick(RKeng::SceneState& scene, ..., float dt) {
    auto& in = scene.input;
    
    if (in.forward)  { /* W */ }
    if (in.backward) { /* S */ }
    if (in.left)     { /* A */ }
    if (in.right)    { /* D */ }
    if (in.jump)     { /* Space */ }
    if (in.run)      { /* Shift */ }
    if (in.crouch)   { /* Ctrl */ }
    
    float yaw   = in.yaw;    // поворот камеры по горизонтали
    float pitch = in.pitch;  // по вертикали
}
```

---

## Важные правила

- **Не включай `CharacterVirtual.h` в плагин** — `JPH_IMPLEMENT_RTTI_VIRTUAL` создаёт глобальные объекты при загрузке DLL → краш до `DllMain`. Используй `api.CreateCharacter()` и `api.SetPlayerVelocity()`.
- **Не линкуй Jolt напрямую в плагин** — синглтоны уже живут в движке. Используй `EngineAPI`.
- **Не включай GLFW в плагин** — читай инпут через `scene.input`, а не через `glfwGetKey`.
- **`mesh.dirty = true`** — если забыл, буфер не обновится и увидишь старую геометрию.

---

## Пример: спавн динамического тела

```cpp
RKeng::RK_DynamicBox box{};
box.cx = 0; box.cy = 5; box.cz = 0;   // позиция
box.hx = 1; box.hy = 1; box.hz = 1;   // half-extents
box.mass = 100.0f;

uint32_t bodyID = api.SpawnDynamicBox(physics, box);

// Каждый тик читаем трансформ
float px, py, pz, qx, qy, qz, qw;
api.GetBodyTransform(physics, bodyID, px, py, pz, qx, qy, qz, qw);
```

Полный справочник API → [API.md](API.md)
