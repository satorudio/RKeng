# RKeng

C++ игровой движок с нуля. Vulkan + Jolt Physics + авторитетный сервер.  
Один разработчик. Астана, Казахстан.

---

## Что это

Самописный 3D движок — не обёртка над Unity/Unreal, не учебный проект.  
Архитектура плагинов: движок собирается один раз, игровые сцены грузятся как DLL без перекомпиляции движка.

Делается потому что корпоративные студии делают игры неправильно.

---

## Стек

| | |
|---|---|
| Рендер | Vulkan (ручная инициализация, instanced rendering, frustum culling) |
| Физика | Jolt Physics (CharacterVirtual, VehicleConstraint, voxel destruction) |
| Сеть | ENet + авторитетный сервер + AntiLAGv1 delta compression |
| Математика | GLM |
| UI | Dear ImGui |
| Аудио | MiniAudio |
| Язык | C++20 |
| Сборка | CMake + Ninja |

---

## Структура

```
RKeng/
├── RKeng/
│   ├── src/              — движок (рендер, физика, ввод, сцены)
│   ├── server/           — авторитетный игровой сервер
│   ├── engine_api/       — публичный контракт (IScenePlugin, EngineAPI)
│   ├── sdk/              — заголовки для разработки плагинов
│   ├── shaders/          — GLSL шейдеры
│   └── lib/              — зависимости (Jolt, glm, imgui, enet, ...)
└── VoxelCarWorld/        — сцена-плагин: открытый мир, машина, воксели
```

Правило архитектуры: каждый логический блок — отдельный файл.  
Оркестраторы (`Engine`, `VulkanContext`) не содержат логики — только вызовы.

---

## Что реализовано

**Рендер**
- Полная цепочка инициализации Vulkan (разбита по файлам ответственности)
- Instanced rendering кубов (тысячи объектов без overhead)
- Frustum culling на CPU (6 плоскостей, AABB тест)
- Depth buffer, правильные семафоры swapchain
- Разрушаемые воксельные стены с отлетающими фрагментами

**Физика**
- Jolt Physics собирается из исходников (нет ABI проблем)
- `CharacterVirtual` — ходьба, бег, приседание, прыжок, двойной прыжок
- `VehicleConstraint` — 4WD машина с разрушаемым voxel корпусом
- Произвольные статические и динамические тела через `EngineAPI`

**Сервер**
- Авторитетный сервер на ENet (UDP)
- `Protocol.h` с `#pragma pack(push,1)`, версионирование, модель угроз
- **AntiLAGv1** — delta compression со статическим словарём и битовой маской полей
  - Стоячий игрок = 2 байта вместо 22
  - `prevSnaps` в `ServerState` (не static, корректно сбрасывается)
  - Буфер рассчитан на 64 игрока без переполнения

**Плагины**
- `ScenePluginLoader` — RAII, некопируемый, cross-platform (LoadLibraryA / dlopen)
- Jolt синглтоны общие между движком и плагином (нет двойной инициализации)
- `CharacterVirtual.h` не включается в DLL — только через `EngineAPI::CreateCharacter`

---

## Роадмап

### AntiLAGv1 — delta compression `[72%]`
Минимальный сетевой трафик без потери точности.
- [x] Битовая маска дельта-снапшота
- [x] EncodeDelta / DecodeDelta
- [x] prevSnaps в ServerState
- [ ] LZ поверх дельты
- [ ] Benchmark: целевой пакет ~8–15 байт

### SeamlessHandoff — multi-server переходы `[8%]`
Игрок перемещается между серверами без разрыва.
- [x] Концепция задокументирована
- [ ] Двойное TCP-соединение в момент перехода
- [ ] Синхронизация стейта между серверами

### DirectToGPU — Vulkan compute pipeline `[15%]`
CPU освобождается от физики и куллинга.
- [x] Instanced rendering (базовый)
- [ ] Frustum culling на GPU (compute shader)
- [ ] Indirect rendering (DrawIndirect)
- [ ] Физика на GPU

### TransportRPhysics — реалистичная физика транспорта `[22%]`
Физика из реальной геометрии игрока.
- [x] VehicleConstraint 4WD
- [x] Voxel destruction + debris
- [ ] Аэродинамика из геометрии
- [ ] Уравнение Циолковского для ракет
- [ ] Weld stress — сварные швы рвутся под нагрузкой

### AutoContentGen — text → 3D asset pipeline `[3%]`
Написал "medieval tower" — получил готовый 3D объект с физикой.
- [ ] Текст → Stable Diffusion → TripoSR → Jolt shape
- [ ] Кэш по хэшу промпта

### RKlang — скриптовый язык `[0%]`
Python-подобный язык компилируется в Jolt команды.
Читерство запрещено на уровне компилятора.

### AIShutUpper — локальный LLM агент `[100% ✓]`
Ollama + per-file batching + djb2 кэш + система штрафов за стабы.

---

## Разработка плагина (сцены)

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
1. `WorldGen::Generate` — пол, препятствия
2. `api.SpawnStaticBox` / `api.SpawnDynamicBox`
3. `ph.physicsSystem->OptimizeBroadPhase()` ← обязательно перед персонажем
4. `api.CreateCharacter`

Полный референс: `sdk/` заголовки.

---

## Сборка

**Требования:** CMake 3.20+, C++20, Vulkan SDK, MinGW-w64 (Windows) / GCC (Linux)

```bash
git clone https://github.com/satorudio/RKeng.git
cd RKeng/RKeng
mkdir build && cd build
cmake -G "Unix Makefiles" ..
ninja
```

Сервер собирается отдельно:
```bash
cd RKeng/server
mkdir build && cd build
cmake -G "Unix Makefiles" ..
ninja
```

---

## Лицензия

Проприетарный. Не открытый исходный код.  
Использование, копирование и распространение без разрешения запрещено.

---

*RKeng — Арсений, Астана, 2025–2026*
