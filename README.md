# 🎮 RKeng — Game Engine

[![C++](https://img.shields.io/badge/C%2B%2B-20-blue?logo=cplusplus)](https://en.wikipedia.org/wiki/C%2B%2B20)
[![CMake](https://img.shields.io/badge/CMake-3.20+-blue?logo=cmake)](https://cmake.org/)
[![License](https://img.shields.io/badge/License-Open%20Source-green)]()
[![Status](https://img.shields.io/badge/Status-Active%20Development-brightgreen)]()

Современный **3D игровой движок** с архитектурой на основе плагинов, построенный на **Vulkan**, **Jolt Physics** и **ECS**.

Оптимизирован для высокопроизводительной интерактивной графики и сложных физических симуляций.

---

## ✨ Основные возможности

### 🎨 Графика
- **Vulkan** — низкоуровневый API для максимальной производительности
- **Шейдеры** — полная поддержка пользовательских шейдеров
- **ImGui** — встроенный UI для отладки и интерфейсов
- **Voxel Rendering** — оптимизированная система вокселей

### 🏗️ Физика
- **Jolt Physics** — современный движок физики AAA-качества
- **Контакты и коллизии** — точное обнаружение и обработка
- **Динамические объекты** — полная поддержка жёстких тел

### 🎯 Архитектура
- **ECS (Entity Component System)** — гибкая и масштабируемая архитектура
- **Plugin System** — загружаемые сцены как DLL-плагины
- **Scene Management** — управление сценами и состояниями
- **Event System** — система событий и взаимодействия

### 🎵 Мультимедиа
- **MiniAudio** — поддержка звука и музыки
- **Assimp** (опционально) — загрузка 3D-моделей
- **STB Image** — работа с изображениями

### 🌐 Сеть
- **ENet** — легкая сетевая коммуникация
- **Многопользовательская поддержка** — готовность к мультиплеру

### ⌨️ Ввод
- **GLFW** — кроссплатформенное управление окном и вводом
- **Клавиатура и мышь** — полная поддержка контроллера
- **Input Polling** — опрос состояния ввода

### 📊 JSON
- **Nlohmann JSON** — сериализация данных и конфигурация
- **Сценарии и уровни** — загрузка через JSON

---

## 🛠️ Технический стек

| Компонент | Технология | Версия |
|-----------|-----------|--------|
| **API Графики** | Vulkan | Latest |
| **Язык** | C++ | 20 |
| **Сборка** | CMake | 3.20+ |
| **Физика** | Jolt Physics | - |
| **ECS** | Встроенная система | - |
| **Окно** | GLFW | 3+ |
| **Математика** | GLM | 1.0.3+ |
| **UI** | Dear ImGui | 1.92.7+ |

---

## 📦 Структура проекта

```
RKeng/
├── RKeng/                    # Основной движок
│   ├── src/
│   │   ├── core/            # Ядро движка
│   │   ├── vulkan/          # Vulkan рендер
│   │   ├── window/          # Система окна (GLFW)
│   │   ├── input/           # Система ввода
│   │   ├── physics/         # Jolt Physics интеграция
│   │   ├── ecs/             # Entity Component System
│   │   ├── scene/           # Управление сценами
│   │   ├── voxel/           # Воксельная графика
│   │   ├── audio/           # MiniAudio интеграция
│   │   ├── network/         # ENet сетевая система
│   │   ├── math/            # Математические типы
│   │   └── utils/           # Утилиты и логирование
│   ├── lib/                 # Внешние библиотеки
│   │   ├── glm/             # Математическая библиотека
│   │   ├── imgui/           # Dear ImGui
│   │   ├── JoltPhysics/     # Движок физики
│   │   ├── enet/            # Сетевая библиотека
│   │   ├── miniaudio/       # Звук
│   │   ├── stb/             # Обработка изображений
│   │   └── assimp/          # Загрузчик моделей (опционально)
│   ├── shaders/             # GLSL/SPIR-V шейдеры
│   ├── engine_api/          # Публичный API для плагинов
│   ├── sdk/                 # SDK для разработки плагинов
│   └── CMakeLists.txt
│
├── VoxelCarWorld/           # Пример сцены (DLL-плагин)
│   ├── src/
│   │   └── OpenCarWorld.cpp # Реализация сцены
│   └── CMakeLists.txt
│
├── build/                   # Директория сборки (создаётся)
├── CMakeLists.txt          # Главный конфиг сборки
├── .gitignore
└── README.md
```

---

## 🚀 Быстрый старт

### Требования

- **CMake** 3.20 или выше
- **C++ компилятор** с поддержкой C++20
- **Vulkan SDK** (LunarG)
- **GLFW** (libglfw3-dev)
- **Python 3** (для утилит SDK)

### Linux/macOS

```bash
# Клонирование
git clone https://github.com/satorudio/RKeng.git
cd RKeng

# Установка зависимостей (Ubuntu/Debian)
sudo apt-get install vulkan-tools libvulkan-dev libglfw3-dev libglm-dev cmake

# Сборка
mkdir build && cd build
cmake ..
cmake --build . --config Release

# Запуск
./RKeng
```

### Windows (MinGW)

```bash
# Сборка
mkdir build && cd build
cmake -G "Unix Makefiles" ..
cmake --build . --config Release

# Запуск
RKeng.exe
```

### Windows (MSVC)

```bash
# Сборка
mkdir build && cd build
cmake -G "Visual Studio 17 2022" ..
cmake --build . --config Release

# Запуск
Release/RKeng.exe
```

---

## 🎮 Использование

### Запуск движка

```cpp
// Основной цикл управляется из main.cpp
// RKengCore.dll загружает все компоненты и сцены
```

### Создание собственной сцены (плагин DLL)

1. **Используйте SDK** в `sdk/` директории
2. **Реализуйте интерфейс** `IScenePlugin.h`
3. **Линкуйте к** `libRKengCore.dll.a`
4. **Поместите DLL** в `build/` директорию

Пример находится в `VoxelCarWorld/`

---

## ⚙️ Конфигурация сборки

### Опциональные компоненты

Включение Assimp для загрузки моделей:
```bash
cmake -DASSIMP_ROOT=/path/to/assimp ..
```

Выбор сцены по умолчанию:
```bash
cmake -DRK_DEFAULT_SCENE="MyScene" ..
```

### Флаги компиляции

| Флаг | Значение |
|------|---------|
| `RK_DEBUG` | Режим отладки (Debug конфиг) |
| `RK_RELEASE` | Режим релиза (Release конфиг) |
| `RK_JOLT_ENABLED` | Физика включена |
| `RK_IMGUI_ENABLED` | UI отладки включен |
| `RK_ASSIMP_ENABLED` | Загрузка моделей включена |

---

## 🔌 Plugin System

### Архитектура DLL-плагинов

```
Engine (RKengCore.dll)
    ├── Jolt Physics Singleton
    ├── Vulkan Renderer
    ├── Scene Manager
    └── Input System

Scene DLL Plugin
    ├── Loads IScenePlugin interface
    ├── Accesses Engine via EngineAPI
    └── Own copy of Jolt (isolated from engine)
```

**Преимущества:**
- Гячей загрузка/выгрузка сцен
- Изоляция сцен друг от друга
- Возможность хотрелоада

---

## 📝 Примеры

### Использование Physics

```cpp
#include "physics/PhysicsState.h"

// Сцена получает доступ к физике через API
auto& physics = engine->GetPhysicsState();
physics.AddRigidBody(...);
```

### Entity Component System

```cpp
#include "ecs/ECS.h"

auto entity = ecs.CreateEntity();
entity.AddComponent<TransformComponent>();
entity.AddComponent<VelocityComponent>();
```

---

## 🐛 Отладка

### Встроенный ImGui UI

Движок включает Dear ImGui для отладки:
- Визуализация сцены
- Профилирование производительности
- Инспектор сущностей

### Логирование

```cpp
#include "utils/Logger.h"

RK_LOG("Message");
RK_WARN("Warning");
RK_ERROR("Error");
```

---

## 📊 Характеристики

- **91.1%** C++
- **7.7%** CMake
- **1.2%** Прочее

---

## 🤝 Контрибьютинг

Приветствуются pull-requests! Перед отправкой убедитесь:
- Код компилируется без ошибок
- Используется C++20 стиль
- Документация обновлена

---

## 📄 Лицензия

Открытый исходный код. Используется в образовательных и коммерческих целях.

---

## 📬 Контакты и поддержка

- **GitHub Issues** — для сообщений об ошибках
- **Discussions** — для общих вопросов и идей

---

## 🎯 Дорожная карта

- ✅ Основной движок и архитектура
- ✅ Vulkan рендер
- ✅ Jolt Physics интеграция
- ✅ Plugin System
- 🔄 **В разработке:** Расширенный UI editor
- 🔄 **В разработке:** Встроенный пакетчик ассетов
- 📋 **Планируется:** Поддержка Lua scripting
- 📋 **Планируется:** Сетевая синхронизация

---

**Создано с ❤️ для разработчиков игр**

*RKeng — выбор современного разработчика для создания интерактивного контента.*
