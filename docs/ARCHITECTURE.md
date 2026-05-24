# Архитектура RKeng

## Обзор

```
RKeng.exe          ← тонкий лончер (~50 строк)
    └── RKengCore  ← движок (Vulkan, Jolt, Input, Network)
            └── YourScene.dll  ← игровая логика (плагин)
                    └── EngineAPI → обратно в RKengCore
```

Сервер — отдельный бинарник, общается с движком по UDP.

---

## Правило одного файла

Каждая логическая единица — отдельный `.cpp` / `.h`.  
Оркестраторы (`Engine.cpp`, `VulkanContext.cpp`, `main.cpp`) не содержат логики — только вызовы.

Примеры:
- `VulkanInstanceCreate.cpp` — только `vkCreateInstance`
- `VulkanDeviceSelect.cpp` — только выбор физического устройства
- `PhysicsTick.cpp` — только шаг физики
- `FrameTick.cpp` — только один кадр рендера

---

## Модули движка

### core/
| Файл | Роль |
|---|---|
| `Engine.cpp` | Точка входа движка |
| `EngineInit/Loop/Shutdown` | Жизненный цикл |
| `FrameTick.cpp` | Один кадр (физика → логика → рендер) |
| `ScenePluginLoader.h` | Загрузка/выгрузка DLL |
| `SceneRegistry.cpp` | Реестр встроенных сцен |
| `SceneState.h` | Всё состояние текущей сцены |

### vulkan/
20+ файлов — каждый этап инициализации отдельно:  
`VulkanInstanceCreate` → `VulkanDeviceSelect` → `VulkanSwapchainCreate` → ...

### physics/
`PhysicsInit` / `PhysicsTick` / `PhysicsShutdown` / `PhysicsState`  
Jolt собирается из исходников — ABI гарантирован.

### server/
Авторитетный сервер на raw UDP.  
`Protocol.h` — общий между клиентом и сервером.  
`PacketDelta.h` — delta compression.

---

## SceneState

Главная структура данных которую движок передаёт плагину каждый тик.

```cpp
struct SceneState {
    PlayerState player;      // позиция, скорость, флаги
    InputState  input;       // WASD, мышь, прыжок
    SceneMesh   sceneMesh;   // геометрия — плагин пишет, движок рендерит
    DVec3       originShift; // для больших миров (float precision fix)
    float       deltaTime;
    float       totalTime;
    GLFWwindow* windowHandle;
    std::array<Vec4, 6> frustumPlanes;  // для frustum culling в плагине
    // ...Jolt BodyID для пола, стен, worldStaticBodyIDs
};
```

### SceneMesh

Движок рендерит `sceneMesh` каждый кадр без вопросов.  
Плагин пишет вершины → ставит `dirty = true` → движок заливает в GPU.

Формат вершины: `pos(xyz) + color(rgb) + normal(xyz)` = 9 floats  
Формат инстанса: `mat4(16) + color(3) + wireframe(1)` = 20 floats

---

## DLL плагины

```
движок запускается
    → ScenePluginLoader::Load("VoxelCarWorld.dll")
        → GetProcAddress("RK_CreateScene")
            → plugin->OnLoad(scene, physics, api)
                → каждый кадр: plugin->OnTick(scene, physics, dt)
    → при выходе: plugin->OnUnload → RK_DestroyScene
```

**Почему не линкуем Jolt в DLL:**  
`JPH_IMPLEMENT_RTTI_VIRTUAL` в Jolt заголовках создаёт глобальные объекты при загрузке DLL — это вызывает краш до `DllMain`. Поэтому Jolt живёт только в движке, плагин получает доступ через `EngineAPI`.

---

## Сеть

```
Клиент (RKengCore) ←→ UDP ←→ Сервер (RKengServer)
```

- Протокол бинарный, поверх raw UDP
- Delta compression: только изменившиеся поля
- `Protocol.h` шарится между клиентом и сервером
- Сервер авторитетный — клиент не доверяет локальному состоянию
