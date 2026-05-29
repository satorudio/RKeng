# LuaScene

DLL-сцена для RKeng. Вместо C++ реализует `IScenePlugin` поверх Lua (sol2).

## Зависимости

Всё кладётся в `RKeng/lib/` — рядом с glm, JoltPhysics и т.д.

### Lua 5.4

```
RKeng/lib/lua/
  include/
    lua.h
    luaconf.h
    lualib.h
    lauxlib.h
  lib/
    liblua.a     (MinGW/Linux)
```

Windows/MinGW: скачай с https://luabinaries.sourceforge.net/ → `lua-5.4.x_Win64_mingw13_bin.zip`  
Linux: `sudo apt install liblua5.4-dev`  (тогда find_package сам найдёт)

### sol2 (header-only)

```
RKeng/lib/sol2/
  include/
    sol/
      sol.hpp
      ...
```

Скачай последний релиз с https://github.com/ThePhD/sol2/releases → `sol2-amalgamation.zip`

## Сборка

```bash
cd bore/LuaScene
cmake -B build -DRKENG_DIR=../RKeng
cmake --build build
```

DLL и `scene.lua` автоматически копируются в `RKeng/build/`.

## Запуск

```bash
# Дефолтный скрипт — scene.lua рядом с exe
./RKeng.exe LuaScene.dll

# Или другой скрипт:
RK_LUA_SCENE=myscene.lua ./RKeng.exe LuaScene.dll
```

## API скрипта

```lua
function on_load()   end   -- вызывается один раз при загрузке сцены
function on_tick(dt) end   -- вызывается каждый кадр, dt в секундах
function on_unload() end   -- вызывается при выгрузке
```

Все функции движка — в таблице `Engine`:

| Функция | Описание |
|---|---|
| `Engine.log_info/warn/error(msg)` | Лог |
| `Engine.spawn_static_box(cx,cy,cz, hx,hy,hz [,is_sensor])` | Статический бокс → bodyID |
| `Engine.spawn_static_box_rot(cx,cy,cz, hx,hy,hz, rot_y, rot_x)` | Со вращением → bodyID |
| `Engine.spawn_dynamic_box(cx,cy,cz, hx,hy,hz [,mass,...])` | Динамический бокс → bodyID |
| `Engine.destroy_body(bodyID)` | Удалить тело |
| `Engine.get_body_transform(bodyID)` | → px,py,pz, qx,qy,qz,qw |
| `Engine.create_character([sx,sy,sz,...])` | Капсула персонажа → bool |
| `Engine.set_player_velocity(vx,vy,vz)` | |
| `Engine.get_player_velocity()` | → vx,vy,vz |
| `Engine.get_gravity_y()` | → float |
| `Engine.spawn_vehicle({...})` | Машина → handle |
| `Engine.set_vehicle_input(h, throttle, brake, steer [,handbrake])` | |
| `Engine.get_vehicle_transform(h)` | → px,py,pz, qx,qy,qz,qw, vx,vy,vz |
| `Engine.destroy_vehicle(h)` | |
| `Engine.optimize_broadphase()` | Обязательно после on_load! |
| `Engine.input` | InputState (read-only) |

### InputState поля
`forward, backward, left, right, jump, jump_pressed, crouch, run,`  
`mouse_dx, mouse_dy, yaw, pitch`
