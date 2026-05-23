# RKeng

Игровой движок на C++ с нуля.

---

## Стек

<details>
<summary>Показать</summary>

Vulkan · Jolt Physics · Custom UDP Protocol · GLM · Dear ImGui · MiniAudio · C++20

</details>

---

## Возможности

<details>
<summary>Рендер</summary>

Низкоуровневый Vulkan рендер с instanced rendering, frustum culling и разрушаемой воксельной геометрией.

</details>

<details>
<summary>Физика</summary>

Полная интеграция Jolt Physics — персонажи, транспорт, разрушаемые объекты, произвольные тела.

</details>

<details>
<summary>Сеть</summary>

Авторитетный сервер на кастомном бинарном протоколе поверх UDP с delta compression.

</details>

<details>
<summary>Плагины</summary>

Игровые сцены — отдельные DLL. Движок собирается один раз, сцены грузятся и выгружаются без рестарта.

</details>

---

## Роадмап

<details>
<summary>Показать</summary>

**AntiLAGv1** — минимизация сетевого трафика  
**SeamlessHandoff** — бесшовные переходы между серверами  
**DirectToGPU** — вынос вычислений в GPU compute  
**TransportRPhysics** — физика транспорта из произвольной геометрии  
**AutoContentGen** — генерация игровых ассетов из текста  
**RKlang** — скриптовый язык с ограничениями на уровне компилятора  
**AIShutUpper** ✓ — агент кодогенерации на локальном LLM

</details>

---

## Разработка сцены

<details>
<summary>Показать</summary>

Скопируй `sdk/`, реализуй `IScenePlugin`, экспортируй фабрику:

```cpp
class MyScene : public RKeng::IScenePlugin {
public:
    const char* GetName() const override { return "my_scene"; }
    void OnLoad(RKeng::SceneState&, RKeng::PhysicsState&, const RKeng::EngineAPI&) override;
    void OnTick(RKeng::SceneState&, RKeng::PhysicsState&, float dt) override;
};

extern "C" {
    RK_EXPORT RKeng::IScenePlugin* RK_CreateScene()              { return new MyScene(); }
    RK_EXPORT void                 RK_DestroyScene(IScenePlugin* p) { delete p; }
}
```

</details>

---

## Сборка

<details>
<summary>Показать</summary>

**Требования:** CMake 3.20+, C++20, Vulkan SDK

```bash
git clone https://github.com/satorudio/RKeng.git
cd RKeng/RKeng && mkdir build && cd build
cmake -G "Unix Makefiles" .. && ninja
```

</details>

---

## Лицензия

Проприетарный. Использование и распространение без разрешения запрещено.
