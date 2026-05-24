# EngineAPI — справочник

`EngineAPI` передаётся плагину в `OnLoad`. Храни как const ref или скопируй в поле класса.

```cpp
class MyScene : public RKeng::IScenePlugin {
    RKeng::EngineAPI m_api;  // копия на весь lifetime сцены
public:
    void OnLoad(..., const RKeng::EngineAPI& api) override {
        m_api = api;
    }
};
```

---

## Логгер

```cpp
api.LogInfo ("текст");   // [INFO]
api.LogWarn ("текст");   // [WARN]
api.LogError("текст");   // [ERROR]
```

---

## Физика — статические тела

```cpp
// Простой статик (без ротации)
RKeng::RK_BoxBody floor{};
floor.position    = { 0, -1, 0 };
floor.halfExtents = { 50, 1, 50 };
uint32_t id = api.SpawnStaticBox(physics, floor);

// Статик с ротацией
RKeng::RK_StaticBox ramp{};
ramp.cx = 0;  ramp.cy = 2;  ramp.cz = 0;   // центр
ramp.hx = 5;  ramp.hy = 0.2f; ramp.hz = 5; // half-extents
ramp.rotX = 0.3f;   // радианы
ramp.rotY = 0.0f;
uint32_t id2 = api.SpawnStaticBoxRot(physics, ramp);
```

---

## Физика — динамические тела

```cpp
RKeng::RK_DynamicBox box{};
box.cx = 0; box.cy = 10; box.cz = 0;
box.hx = 1; box.hy = 1;  box.hz = 1;
box.mass           = 100.0f;
box.linearDamping  = 0.05f;   // сопротивление линейному движению
box.angularDamping = 0.4f;    // сопротивление вращению
box.friction       = 0.5f;

uint32_t bodyID = api.SpawnDynamicBox(physics, box);
```

### Получить трансформ

```cpp
float px, py, pz;         // позиция
float qx, qy, qz, qw;    // кватернион
bool ok = api.GetBodyTransform(physics, bodyID,
                               px, py, pz,
                               qx, qy, qz, qw);
```

### Уничтожить тело

```cpp
api.DestroyBody(physics, bodyID);
```

---

## Физика — персонаж

> ⚠️ Не включай `CharacterVirtual.h` в плагин — краш при загрузке DLL.  
> Используй API-функции ниже.

```cpp
// OnLoad — создать персонажа
RKeng::RK_CharacterDesc desc{};
desc.spawnX           = 0;
desc.spawnY           = 5;
desc.spawnZ           = 0;
desc.capsuleHalfHeight = 0.9f;
desc.capsuleRadius     = 0.35f;
desc.maxSlopeAngleDeg  = 45.f;

bool ok = api.CreateCharacter(physics, desc);
// Вызывать ПОСЛЕ SpawnStaticBox + оптимизации broadphase
```

```cpp
// OnTick — двигать персонажа
api.SetPlayerVelocity(physics, vx, vy, vz);

float vx, vy, vz;
api.GetPlayerVelocity(physics, vx, vy, vz);

float g = api.GetGravityY(physics);  // обычно -9.81
```

---

## Версии API

| Версия | Что добавлено |
|---|---|
| 3 | GetBodyTransform, SetPlayerVelocity, GetPlayerVelocity, GetGravityY |
| 4 | CreateCharacter |
| 5 | Jolt синглтоны (joltFactory, joltAllocate, ...) |

Проверить версию:
```cpp
if (api.engineVersion < 4) {
    api.LogError("нужен движок v4+");
    return;
}
```
