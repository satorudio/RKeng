# RKeng

C++ игровой движок с нуля. Vulkan + Jolt Physics + авторитетный сервер.

---

## Что это

Самописный 3D движок — не обёртка над Unity/Unreal, не учебный проект.  
Архитектура плагинов: движок собирается один раз, игровые сцены грузятся как DLL без перекомпиляции движка.

---

## Стек

<details>
<summary>Показать</summary>

| | |
|---|---|
| Рендер | Vulkan |
| Физика | Jolt Physics |
| Сеть | Raw sockets (custom protocol) |
| Математика | GLM |
| UI | Dear ImGui |
| Аудио | MiniAudio |
| Язык | C++20 |
| Сборка | CMake + Ninja |

</details>

---

## Архитектура

<details>
<summary>Показать</summary>

```
RKeng/
├── RKeng/
│   ├── src/              — движок (рендер, физика, ввод, сцены)
│   ├── server/           — авторитетный игровой сервер
│   ├── engine_api/       — публичный контракт (IScenePlugin, EngineAPI)
│   ├── sdk/              — заголовки для разработки плагинов
│   ├── shaders/          — GLSL шейдеры
│   └── lib/              — зависимости (Jolt, glm, imgui, ...)
└── VoxelCarWorld/        — сцена-плагин: открытый мир, машина, воксели
```

Правило: каждый логический блок — отдельный файл.  
Оркестраторы (`Engine`, `VulkanContext`) не содержат логики — только вызовы.

</details>

---

## Что реализовано

<details>
<summary>Рендер</summary>

- Полная цепочка инициализации Vulkan (разбита по файлам ответственности)
- Instanced rendering — тысячи объектов без overhead
- Frustum culling на CPU (6 плоскостей, AABB тест)
- Depth buffer, корректные семафоры swapchain
- Разрушаемые воксельные стены с отлетающими фрагментами

</details>

<details>
<summary>Физика</summary>

- Jolt Physics собирается из исходников (нет ABI проблем)
- `CharacterVirtual` — ходьба, бег, приседание, прыжок, двойной прыжок
- `VehicleConstraint` — 4WD машина с разрушаемым voxel корпусом
- Произвольные статические и динамические тела через `EngineAPI`

</details>

<details>
<summary>Сеть</summary>

- Авторитетный сервер на raw UDP сокетах
- Кастомный бинарный протокол (`#pragma pack(push,1)`, версионирование)
- **AntiLAGv1** — delta compression со статическим словарём и битовой маской полей
- Стоячий игрок = 2 байта вместо 22

</details>

<details>
<summary>Плагины</summary>

- `ScenePluginLoader` — RAII, некопируемый, cross-platform
- Jolt синглтоны общие между движком и плагином
- `CharacterVirtual` не включается в DLL — только через `EngineAPI`

</details>

---

## Роадмап

<details>
<summary>Показать</summary>

Роадмап движка — что планируется поддерживать на уровне API и инфраструктуры.  
Конкретная игровая логика (физика транспорта, механики и т.д.) — это уже на стороне сцены-плагина.

**AntiLAGv1** `[72%]` — delta compression сетевых пакетов  
**SeamlessHandoff** `[8%]` — переходы между серверами без разрыва соединения  
**DirectToGPU** `[15%]` — вынос физики и куллинга в Vulkan compute  
**TransportRPhysics** `[22%]` — API для реалистичной физики транспорта из произвольной геометрии  
**AutoContentGen** `[3%]` — пайплайн text → 3D asset → Jolt collision shape  
**RKlang** `[0%]` — скриптовый язык компилируется в команды движка, читерство запрещено на уровне компилятора  
**AIShutUpper** `[100% ✓]` — локальный LLM агент для кодогенерации (Ollama + система штрафов за стабы)

</details>

---

## Разработка плагина

<details>
<summary>Показать</summary>

Скопируй `sdk/`, реализуй `IScenePlugin`, экспортируй фабрику:

```cpp
#include "IScenePlugin.h"
#include "EngineAPI.h"

class MyScene : public RKeng::IScenePlugin {
public:
    const char* GetName() const override { return "my_scene"; }

    void OnLoad(RKeng::SceneState& scene, RKeng::PhysicsState& ph,
                const RKeng::EngineAPI& api) override {
        // спавн физики, WorldGen, etc.
    }

    void OnTick(RKeng::SceneState& scene, RKeng::PhysicsState& ph,
                float dt) override {
        // игровая логика
    }
};

extern "C" {
    RK_EXPORT RKeng::IScenePlugin* RK_CreateScene()  { return new MyScene(); }
    RK_EXPORT void RK_DestroyScene(RKeng::IScenePlugin* p) { delete p; }
}
```

Порядок инициализации в `OnLoad`:
1. `WorldGen::Generate`
2. `api.SpawnStaticBox` / `api.SpawnDynamicBox`
3. `ph.physicsSystem->OptimizeBroadPhase()` ← обязательно перед персонажем
4. `api.CreateCharacter`

</details>

---

## Сборка

<details>
<summary>Показать</summary>

**Требования:** CMake 3.20+, C++20, Vulkan SDK, MinGW-w64 / GCC

```bash
git clone https://github.com/satorudio/RKeng.git
cd RKeng/RKeng
mkdir build && cd build
cmake -G "Unix Makefiles" ..
ninja
```

Сервер:
```bash
cd RKeng/server
mkdir build && cd build
cmake -G "Unix Makefiles" ..
ninja
```

</details>

---

## Лицензия

Проприетарный. Не открытый исходный код.  
Использование, копирование и распространение без разрешения запрещено.
