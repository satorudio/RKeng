<div align="center">

# RKeng

[![C++20](https://img.shields.io/badge/C%2B%2B-20-ff3c00?style=flat-square&logo=cplusplus)](https://en.cppreference.com/w/cpp/20)
[![Vulkan](https://img.shields.io/badge/Vulkan-rendering-ff3c00?style=flat-square&logo=vulkan)](https://www.vulkan.org/)
[![Jolt](https://img.shields.io/badge/Jolt-physics-ff3c00?style=flat-square)](https://github.com/jrouwe/JoltPhysics)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-ff3c00?style=flat-square)](/docs/BUILD.md)
[![Status](https://img.shields.io/badge/status-active-ff3c00?style=flat-square)]()
[![License](https://img.shields.io/badge/license-Proprietary-ff3c00?style=flat-square)]()

**Игровой движок на C++ с нуля. Никаких абстракций ради абстракций.**

[Сборка](docs/BUILD.md) · [Плагины](docs/PLUGIN.md) · [EngineAPI](docs/API.md) · [Архитектура](docs/ARCHITECTURE.md) · [Роадмап](docs/ROADMAP.md)

</div>

---

## Зачем ещё один движок

| | RKeng | Unity | Unreal | Godot |
|---|---|---|---|---|
| Размер рантайма | ~1 MB | ~200 MB | ~500 MB+ | ~40 MB |
| Vulkan нативно | ✅ | ❌ абстракция | ❌ абстракция | ❌ абстракция |
| Delta compression | ✅ | ❌ | платно | ❌ |
| DLL-плагины без рестарта | ✅ | ❌ | ❌ | ❌ |
| Контроль физтика | ✅ | ❌ | частично | ❌ |
| Кастомный сетевой протокол | ✅ | ❌ | ❌ | ❌ |
| Royalty / подписка | нет | есть | 5% revenue | нет |

---

## Быстрый старт

```bash
git clone https://github.com/satorudio/RKeng.git
cd RKeng/RKeng && mkdir build && cd build
cmake -G "Ninja" .. && ninja
```

Подробнее → [docs/BUILD.md](docs/BUILD.md)

---

## Написать плагин (сцену)

```cpp
#include "sdk/IScenePlugin.h"
#include "sdk/EngineAPI.h"

class MyScene : public RKeng::IScenePlugin {
public:
    const char* GetName() const override { return "my_scene"; }

    void OnLoad(RKeng::SceneState& scene, RKeng::PhysicsState& physics,
                const RKeng::EngineAPI& api) override
    {
        api.LogInfo("MyScene загружена");
        // спавн физики, геометрии — см. docs/API.md
    }

    void OnTick(RKeng::SceneState& scene, RKeng::PhysicsState& physics,
                float dt) override { /* логика каждый кадр */ }
};

extern "C" {
    RK_EXPORT RKeng::IScenePlugin* RK_CreateScene()                 { return new MyScene(); }
    RK_EXPORT void                 RK_DestroyScene(IScenePlugin* p) { delete p; }
}
```

Подробнее → [docs/PLUGIN.md](docs/PLUGIN.md)

---

## Документация

| Файл | Содержание |
|---|---|
| [docs/BUILD.md](docs/BUILD.md) | Сборка движка и плагина на Windows / Linux |
| [docs/PLUGIN.md](docs/PLUGIN.md) | Как писать сцену-плагин с нуля |
| [docs/API.md](docs/API.md) | Справочник EngineAPI — физика, логгер, персонаж |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Внутреннее устройство движка |
| [docs/ROADMAP.md](docs/ROADMAP.md) | Что запланировано |

---

## Лицензия

Проприетарный. Использование без разрешения запрещено.
