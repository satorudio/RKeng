# Сборка RKeng

## Требования

| Инструмент | Версия |
|---|---|
| CMake | 3.20+ |
| C++ компилятор | GCC 12+ / Clang 15+ / MSVC 2022 |
| Vulkan SDK | 1.3+ |
| Ninja | любая |

---

## Windows

```bat
git clone https://github.com/satorudio/RKeng.git
cd RKeng\RKeng
mkdir build && cd build
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release ..
ninja
```

Запуск: `RKeng.exe` появится в `build/`.  
Рядом положи `YourScene.dll` — движок подхватит автоматически.

---

## Linux

```bash
git clone https://github.com/satorudio/RKeng.git
cd RKeng/RKeng && mkdir build && cd build
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release ..
ninja
./RKeng
```

---

## Jolt Physics

Jolt собирается из исходников — никаких prebuilt не нужно.  
`cmake ..` сам подтянет через `add_subdirectory`.  
Если получаешь ABI mismatch — проверь что флаги компилятора совпадают между движком и плагином (особенно `_GLIBCXX_USE_CXX11_ABI`).

---

## Сборка плагина (сцены)

```bash
cd RKeng/VoxelCarWorld && mkdir build && cd build
cmake -G "Ninja" ..
ninja
# результат: VoxelCarWorld.dll / VoxelCarWorld.so
```

Скопируй рядом с `RKeng.exe`. Движок загрузит при старте.

---

## Флаги CMake

| Флаг | По умолчанию | Описание |
|---|---|---|
| `RK_JOLT_ENABLED` | ON | Физика Jolt |
| `RK_SERVER_BUILD` | OFF | Собрать отдельный сервер |
| `CMAKE_BUILD_TYPE` | Debug | Release для продакшена |
