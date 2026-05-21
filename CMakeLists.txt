cmake_minimum_required(VERSION 3.20)
project(RKeng VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
#  PATHS
# ============================================================
set(SRC_DIR      ${CMAKE_SOURCE_DIR}/src)
set(LIB_DIR      ${CMAKE_SOURCE_DIR}/lib)
set(ENGINE_API   ${CMAKE_SOURCE_DIR}/engine_api)

# ============================================================
#  VULKAN
# ============================================================
find_package(Vulkan REQUIRED)

# ============================================================
#  GLFW
# ============================================================
find_package(glfw3 REQUIRED)

# ============================================================
#  GLM 1.0.3
# ============================================================
add_library(rk_glm INTERFACE)
target_include_directories(rk_glm INTERFACE ${LIB_DIR}/glm)
target_compile_definitions(rk_glm INTERFACE
    GLM_FORCE_RADIANS
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_ENABLE_EXPERIMENTAL
)

# ============================================================
#  NLOHMANN JSON
# ============================================================
add_library(rk_json INTERFACE)
if(EXISTS ${LIB_DIR}/json/nlohmann/json.hpp)
    target_include_directories(rk_json INTERFACE ${LIB_DIR}/json)
elseif(EXISTS ${LIB_DIR}/nlohmann/json.hpp)
    target_include_directories(rk_json INTERFACE ${LIB_DIR})
else()
    target_include_directories(rk_json INTERFACE ${LIB_DIR})
    message(STATUS "[RKeng] json.hpp: включаем весь lib/ — используй #include <json.hpp>")
endif()

# ============================================================
#  JOLT PHYSICS
# ============================================================
set(JOLT_DIR ${LIB_DIR}/JoltPhysics)
set(JOLT_LIB ${JOLT_DIR}/libJolt.a)

if(EXISTS ${JOLT_LIB})
    message(STATUS "[RKeng] Jolt: используем precompiled ${JOLT_LIB}")
    add_library(Jolt STATIC IMPORTED GLOBAL)
    set_target_properties(Jolt PROPERTIES
        IMPORTED_LOCATION ${JOLT_LIB}
        INTERFACE_INCLUDE_DIRECTORIES "${JOLT_DIR};${JOLT_DIR}/.."
    )
    set(JOLT_AVAILABLE TRUE)
elseif(EXISTS ${JOLT_DIR}/CMakeLists.txt)
    message(STATUS "[RKeng] Jolt: собираем из исходников ${JOLT_DIR}")
    set(JPH_ENABLE_ASSERTS OFF CACHE BOOL "" FORCE)
    set(JPH_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    set(JPH_BUILD_SAMPLES  OFF CACHE BOOL "" FORCE)
    set(JPH_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    add_subdirectory(${JOLT_DIR} ${CMAKE_BINARY_DIR}/Jolt)
    set(JOLT_AVAILABLE TRUE)
    set_target_properties(Jolt PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${JOLT_DIR};${JOLT_DIR}/.."
    )
else()
    set(JOLT_AVAILABLE FALSE)
    message(WARNING "[RKeng] Jolt не найден — физика отключена")
endif()

# ============================================================
#  ENET (header-only wrapper)
# ============================================================
add_library(rk_enet INTERFACE)
if(EXISTS ${LIB_DIR}/enet/enet.h)
    target_include_directories(rk_enet INTERFACE ${LIB_DIR}/enet)
endif()

# ============================================================
#  MINIAUDIO (header-only)
# ============================================================
add_library(rk_miniaudio INTERFACE)
if(EXISTS ${LIB_DIR}/miniaudio/miniaudio.h)
    target_include_directories(rk_miniaudio INTERFACE ${LIB_DIR}/miniaudio)
endif()

# ============================================================
#  STB (header-only)
# ============================================================
add_library(rk_stb INTERFACE)
if(EXISTS ${LIB_DIR}/stb)
    target_include_directories(rk_stb INTERFACE ${LIB_DIR}/stb)
endif()

# ============================================================
#  IMGUI
# ============================================================
set(IMGUI_DIR ${LIB_DIR}/imgui)
if(EXISTS ${IMGUI_DIR}/imgui.h)
    set(IMGUI_AVAILABLE TRUE)
    set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
    )
    add_library(rk_imgui STATIC ${IMGUI_SOURCES})
    target_include_directories(rk_imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
    target_link_libraries(rk_imgui PRIVATE Vulkan::Vulkan glfw)
    message(STATUS "[RKeng] ImGui found.")
else()
    set(IMGUI_AVAILABLE FALSE)
    message(STATUS "[RKeng] ImGui не найден.")
endif()

# ============================================================
#  ASSIMP (опционально)
# ============================================================
if(DEFINED ASSIMP_ROOT AND EXISTS ${ASSIMP_ROOT}/include/assimp/Importer.hpp)
    add_library(rk_assimp INTERFACE)
    target_include_directories(rk_assimp INTERFACE ${ASSIMP_ROOT}/include)
    target_link_directories(rk_assimp INTERFACE ${ASSIMP_ROOT}/lib)
    set(ASSIMP_AVAILABLE TRUE)
    message(STATUS "[RKeng] Assimp found at ${ASSIMP_ROOT}")
else()
    set(ASSIMP_AVAILABLE FALSE)
    message(STATUS "[RKeng] Assimp не найден. Передай -DASSIMP_ROOT=<path>")
endif()

# ============================================================
#  СЦЕНА ПО УМОЛЧАНИЮ
# ============================================================
if(NOT DEFINED RK_DEFAULT_SCENE_DLL)
    set(RK_DEFAULT_SCENE_DLL "")
endif()
if(NOT DEFINED RK_DEFAULT_SCENE)
    set(RK_DEFAULT_SCENE "template")
endif()

# ============================================================
#  ENGINE SOURCES
# ============================================================
file(GLOB_RECURSE ENGINE_SOURCES
    ${SRC_DIR}/core/*.cpp
    ${SRC_DIR}/vulkan/*.cpp
    ${SRC_DIR}/window/*.cpp
    ${SRC_DIR}/math/*.cpp
    ${SRC_DIR}/audio/*.cpp
    ${SRC_DIR}/network/*.cpp
    ${SRC_DIR}/ecs/*.cpp
    ${SRC_DIR}/utils/*.cpp
    ${SRC_DIR}/physics/*.cpp
    ${SRC_DIR}/input/*.cpp
    ${SRC_DIR}/voxel/*.cpp
    ${SRC_DIR}/scene/SceneTemplate.cpp
    ${SRC_DIR}/scene/SceneLoad.cpp
    ${SRC_DIR}/scene/PlayerMove.cpp
    ${SRC_DIR}/scene/CarState.cpp
    ${SRC_DIR}/scene/CarLoad.cpp
    ${SRC_DIR}/scene/CarMesh.cpp
    ${SRC_DIR}/scene/CarInputPoll.cpp
    ${SRC_DIR}/scene/CarTick.cpp
    ${SRC_DIR}/scene/WorldGen.cpp
)

# ============================================================
#  RKengCore — SHARED DLL (весь движок + Jolt живут здесь)
#
#  Зачем SHARED, а не exe с --export-all-symbols:
#    --export-all-symbols заставляет ld экспортировать internal-
#    символы Jolt (lambda objects), у которых тип "object" (4),
#    а не "function" (3) — MinGW ld падает с "symbol wrong type".
#    SHARED DLL экспортирует только то что помечено RK_API/__declspec
#    (dllexport), lambda-объекты остаются приватными.
#    Плагины-сцены линкуются к libRKengCore.dll.a и получают
#    доступ к Jolt-синглтонам автоматически через единственный
#    инстанс DLL — void* в EngineAPI больше не нужны.
# ============================================================
add_library(RKengCore SHARED ${ENGINE_SOURCES})

# MinGW LTO: при сборке SHARED с LTO ld пытается экспортировать lambda-объекты
# из libJolt.a (тип символа object/4, не function/3) → "symbol wrong type".
# --exclude-libs,libJolt.a предотвращает это.
# Плагины получают JPH-символы из своей копии libJolt.a (см. VoxelCarWorld/CMakeLists.txt).
if(WIN32 AND NOT MSVC)
    target_link_options(RKengCore PRIVATE "-Wl,--exclude-libs,libJolt.a")
endif()


target_include_directories(RKengCore PRIVATE
    ${SRC_DIR}
    ${LIB_DIR}
    ${ENGINE_API}
    ${Vulkan_INCLUDE_DIRS}
)

target_compile_definitions(RKengCore PRIVATE
    RK_ENGINE_BUILD
    RK_DEFAULT_SCENE_DLL="${RK_DEFAULT_SCENE_DLL}"
    RK_DEFAULT_SCENE="${RK_DEFAULT_SCENE}"
    $<$<CONFIG:Debug>:   RK_DEBUG>
    $<$<CONFIG:Release>: RK_RELEASE>
)

target_link_libraries(RKengCore PRIVATE
    Vulkan::Vulkan
    glfw
    rk_glm
    rk_json
    rk_enet
    rk_miniaudio
    rk_stb
)

if(JOLT_AVAILABLE)
    target_link_libraries(RKengCore PRIVATE Jolt)
    target_include_directories(RKengCore PRIVATE ${JOLT_DIR} ${JOLT_DIR}/Build/..)
    target_compile_definitions(RKengCore PRIVATE RK_JOLT_ENABLED)
endif()

if(IMGUI_AVAILABLE)
    target_link_libraries(RKengCore PRIVATE rk_imgui)
    target_compile_definitions(RKengCore PRIVATE RK_IMGUI_ENABLED)
endif()

if(ASSIMP_AVAILABLE)
    target_link_libraries(RKengCore PRIVATE rk_assimp assimp)
    target_compile_definitions(RKengCore PRIVATE RK_ASSIMP_ENABLED)
endif()

if(WIN32)
    target_link_libraries(RKengCore PRIVATE ws2_32 winmm)
endif()

# ============================================================
#  BUILD FLAGS
# ============================================================
if(MSVC)
    target_compile_options(RKengCore PRIVATE
        $<$<CONFIG:Debug>:   /Od /Zi /W4>
        $<$<CONFIG:Release>: /O2 /W3>
    )
else()
    target_compile_options(RKengCore PRIVATE
        $<$<CONFIG:Debug>:   -g -O0 -Wall -Wextra>
        $<$<CONFIG:Release>: -O3 -march=native>
    )
endif()

# ============================================================
#  RKeng.exe — тонкий лончер, только загружает RKengCore.dll
# ============================================================
add_executable(RKeng ${SRC_DIR}/main.cpp)

target_link_libraries(RKeng PRIVATE RKengCore)

target_include_directories(RKeng PRIVATE
    ${SRC_DIR}
    ${ENGINE_API}
)

if(MSVC)
    target_compile_options(RKeng PRIVATE
        $<$<CONFIG:Debug>:   /Od /Zi /W4>
        $<$<CONFIG:Release>: /O2 /W3>
    )
else()
    target_compile_options(RKeng PRIVATE
        $<$<CONFIG:Debug>:   -g -O0>
        $<$<CONFIG:Release>: -O3>
    )
endif()

# RKengCore.dll должна лежать рядом с exe
add_custom_command(TARGET RKeng POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:RKengCore>
        $<TARGET_FILE_DIR:RKeng>
    COMMENT "[RKeng] Copying RKengCore.dll next to RKeng.exe"
)

# ============================================================
#  КОПИРУЕМ ШЕЙДЕРЫ
# ============================================================
if(EXISTS ${CMAKE_SOURCE_DIR}/shaders)
    add_custom_command(TARGET RKeng POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/shaders
            $<TARGET_FILE_DIR:RKeng>/shaders
        COMMENT "Copying shaders to build directory"
    )
endif()

# ============================================================
#  SDK — публичные заголовки для DLL-сцен
#  Сцены линкуются к: libRKengCore.dll.a (import lib из build/)
# ============================================================
set(SDK_DIR ${CMAKE_SOURCE_DIR}/sdk)

add_custom_command(TARGET RKengCore POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/engine_api/IScenePlugin.h
        ${CMAKE_SOURCE_DIR}/engine_api/EngineAPI.h
        ${SDK_DIR}/
    COMMENT "[RKeng] SDK: copying engine_api headers"
)

# FlattenSDK: копируем заголовки из src/ в sdk/ заменяя все относительные пути
# (#include "../../foo/Bar.h" → #include "Bar.h"). Python вместо CMake-скрипта
# чтобы избежать многоуровневого CMake regex-эскейпинга.
set(SDK_FLATTEN_SCRIPT ${CMAKE_BINARY_DIR}/FlattenSDK.py)
file(WRITE ${SDK_FLATTEN_SCRIPT} [=[
import re, sys, os
from pathlib import Path

src_dir = Path(sys.argv[1])
sdk_dir = Path(sys.argv[2])
sdk_dir.mkdir(parents=True, exist_ok=True)

headers = [
    "scene/CarState.h",
    "scene/CarInputPoll.h",
    "scene/CarLoad.h",
    "scene/CarMesh.h",
    "scene/CarTick.h",
    "scene/PlayerMove.h",
    "scene/SceneLoad.h",
    "core/SceneState.h",
    "voxel/VoxelWall.h",
    "vulkan/VoxelWallBuffer.h",
    "scene/WorldGen.h",
    "math/MathTypes.h",
    "physics/PhysicsState.h",
    "physics/RKContactListener.h",
    "utils/Logger.h",
]

# Заменяем #include "any/path/File.h" -> #include "File.h"
# Не трогаем <system> includes и уже плоские "File.h"
pattern = re.compile(r'#include "(?:[^/"]+/)+([^/"]+\.h)"')

for rel in headers:
    src = src_dir / rel
    dst = sdk_dir / Path(rel).name
    text = src.read_text(encoding="utf-8")
    text = pattern.sub(r'#include "\1"', text)
    dst.write_text(text, encoding="utf-8")

print(f"[RKeng] SDK headers synced to {sdk_dir}")
]=])

add_custom_command(TARGET RKengCore POST_BUILD
    COMMAND python3 ${SDK_FLATTEN_SCRIPT} ${SRC_DIR} ${SDK_DIR}
    COMMENT "[RKeng] SDK: syncing scene headers"
)

# ============================================================
#  SERVER
# ============================================================
if(EXISTS ${CMAKE_SOURCE_DIR}/server/CMakeLists.txt)
    add_subdirectory(server)
    message(STATUS "[RKeng] Server: включён (RKengServer)")
else()
    message(STATUS "[RKeng] Server: CMakeLists.txt не найден, пропускаем")
endif()

message(STATUS "================================")
message(STATUS " RKeng configuration summary")
message(STATUS "  Vulkan      : ${Vulkan_VERSION}")
message(STATUS "  Jolt        : ${JOLT_AVAILABLE}")
message(STATUS "  ImGui       : ${IMGUI_AVAILABLE}")
message(STATUS "  Assimp      : ${ASSIMP_AVAILABLE}")
message(STATUS "  Default DLL : '${RK_DEFAULT_SCENE_DLL}'")
message(STATUS "  Default scene: '${RK_DEFAULT_SCENE}'")
message(STATUS "================================")
