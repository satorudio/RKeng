cmake_minimum_required(VERSION 3.20)
project(RKeng VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
#  PATHS
# ============================================================
set(SRC_DIR  ${CMAKE_SOURCE_DIR}/src)
set(LIB_DIR  ${CMAKE_SOURCE_DIR}/lib)

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
#  Если lib/JoltPhysics/libJolt.a есть — используем как IMPORTED (быстро).
#  Если нет — собираем из исходников (медленно, только первый раз).
#  После первой сборки из исходников скопируй:
#    build/Jolt/libJolt.a  ->  lib/JoltPhysics/libJolt.a
#  и следующие сборки будут быстрыми.
# ============================================================
set(JOLT_DIR ${LIB_DIR}/JoltPhysics)
set(JOLT_LIB ${JOLT_DIR}/libJolt.a)

if(EXISTS ${JOLT_LIB})
    message(STATUS "[RKeng] Jolt: используем precompiled ${JOLT_LIB}")
    add_library(Jolt STATIC IMPORTED GLOBAL)
    set_target_properties(Jolt PROPERTIES
        IMPORTED_LOCATION             ${JOLT_LIB}
        INTERFACE_INCLUDE_DIRECTORIES ${JOLT_DIR}
    )
    set(JOLT_AVAILABLE TRUE)
elseif(EXISTS ${JOLT_DIR}/Jolt/Jolt.h)
    message(STATUS "[RKeng] Jolt: собираем из исходников через ExternalProject...")
    include(ExternalProject)
    set(JOLT_BUILD_DIR ${CMAKE_BINARY_DIR}/Jolt)
    set(JOLT_OUTPUT_LIB ${JOLT_BUILD_DIR}/libJolt.a)
    ExternalProject_Add(JoltBuild
        SOURCE_DIR        ${JOLT_DIR}/Build
        BINARY_DIR        ${JOLT_BUILD_DIR}
        CMAKE_ARGS
            -DCMAKE_BUILD_TYPE=Release
            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
            -DDEBUG_RENDERER_IN_DEBUG_AND_RELEASE=OFF
            -DDEBUG_RENDERER_IN_DISTRIBUTION=OFF
            -DPROFILER_IN_DEBUG_AND_RELEASE=OFF
            -DPROFILER_IN_DISTRIBUTION=OFF
            -DENABLE_OBJECT_STREAM=OFF
            -DTARGET_UNIT_TESTS=OFF
            -DTARGET_HELLO_WORLD=OFF
            -DTARGET_SAMPLES=OFF
            -DTARGET_VIEWER=OFF
        BUILD_BYPRODUCTS  ${JOLT_OUTPUT_LIB}
        INSTALL_COMMAND   ""
    )
    add_library(Jolt STATIC IMPORTED GLOBAL)
    set_target_properties(Jolt PROPERTIES
        IMPORTED_LOCATION             ${JOLT_OUTPUT_LIB}
        INTERFACE_INCLUDE_DIRECTORIES ${JOLT_DIR}
    )
    add_dependencies(Jolt JoltBuild)
    set(JOLT_AVAILABLE TRUE)
else()
    set(JOLT_AVAILABLE FALSE)
    message(WARNING "[RKeng] Jolt не найден — физика отключена.")
endif()

# ============================================================
#  ENET
# ============================================================
add_library(rk_enet INTERFACE)
if(EXISTS ${LIB_DIR}/enet/enet.h)
    target_include_directories(rk_enet INTERFACE ${LIB_DIR}/enet)
else()
    target_include_directories(rk_enet INTERFACE ${LIB_DIR})
endif()

# ============================================================
#  MINIAUDIO
# ============================================================
add_library(rk_miniaudio INTERFACE)
target_include_directories(rk_miniaudio INTERFACE ${LIB_DIR})
target_compile_definitions(rk_miniaudio INTERFACE
    MINIAUDIO_IMPLEMENTATION
)

# ============================================================
#  STB_IMAGE
# ============================================================
add_library(rk_stb INTERFACE)
target_include_directories(rk_stb INTERFACE ${LIB_DIR})

# ============================================================
#  DEAR IMGUI 1.92.7
# ============================================================
set(IMGUI_DIR ${LIB_DIR}/imgui)
if(EXISTS ${IMGUI_DIR}/imgui.cpp)
    file(GLOB IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
    )
    add_library(rk_imgui STATIC ${IMGUI_SOURCES})
    target_include_directories(rk_imgui PUBLIC
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        ${Vulkan_INCLUDE_DIRS}
    )
    set(IMGUI_AVAILABLE TRUE)
    message(STATUS "[RKeng] ImGui 1.92.7 found.")
else()
    set(IMGUI_AVAILABLE FALSE)
    message(STATUS "[RKeng] ImGui не найден в ${IMGUI_DIR} — пропускаем.")
endif()

# ============================================================
#  ASSIMP
# ============================================================
if(NOT DEFINED ASSIMP_ROOT)
    set(ASSIMP_ROOT "${LIB_DIR}/assimp")
endif()
if(EXISTS "${ASSIMP_ROOT}/include/assimp/scene.h")
    add_library(rk_assimp INTERFACE)
    target_include_directories(rk_assimp INTERFACE "${ASSIMP_ROOT}/include")
    target_link_directories(rk_assimp INTERFACE "${ASSIMP_ROOT}/lib")
    set(ASSIMP_AVAILABLE TRUE)
    message(STATUS "[RKeng] Assimp found at ${ASSIMP_ROOT}")
else()
    set(ASSIMP_AVAILABLE FALSE)
    message(STATUS "[RKeng] Assimp не найден — модели отключены. Передай -DASSIMP_ROOT=<path>")
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
    # src/scene/ вне движка - сцены это отдельные DLL через ScenePluginLoader
    ${SRC_DIR}/input/*.cpp
)

add_executable(RKeng ${SRC_DIR}/main.cpp ${ENGINE_SOURCES})

# SDK - публичные заголовки для проектов сцен
set(SDK_DIR ${CMAKE_SOURCE_DIR}/sdk)
set(ENGINE_API_DIR ${CMAKE_SOURCE_DIR}/engine_api)
set(SDK_SCENE_HEADERS
    ${ENGINE_API_DIR}/IScenePlugin.h
    ${ENGINE_API_DIR}/EngineAPI.h
    ${SRC_DIR}/core/SceneState.h
    ${SRC_DIR}/scene/CarState.h
    ${SRC_DIR}/voxel/VoxelWall.h
    ${SRC_DIR}/physics/PhysicsState.h
    ${SRC_DIR}/math/MathTypes.h
)
foreach(HDR ${SDK_SCENE_HEADERS})
    if(EXISTS ${HDR})
        add_custom_command(TARGET RKeng POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${HDR} ${SDK_DIR}/
        )
    endif()
endforeach()
message(STATUS "[RKeng] SDK will be exported to ${SDK_DIR}/ after build")

# ============================================================
#  INCLUDES
# ============================================================
target_include_directories(RKeng PRIVATE
    ${SRC_DIR}
    ${LIB_DIR}
    ${ENGINE_API_DIR}
    ${Vulkan_INCLUDE_DIRS}
)

# ============================================================
#  LINK
# ============================================================
target_link_libraries(RKeng PRIVATE
    Vulkan::Vulkan
    glfw
    rk_glm
    rk_json
    rk_enet
    rk_miniaudio
    rk_stb
)

if(JOLT_AVAILABLE)
    target_link_libraries(RKeng PRIVATE Jolt)
    target_include_directories(RKeng PRIVATE ${JOLT_DIR} ${JOLT_DIR}/Build/..)
    target_compile_definitions(RKeng PRIVATE RK_JOLT_ENABLED)
    # JPH_DEBUG_RENDERER, JPH_PROFILE_ENABLED, JPH_OBJECT_STREAM — НЕ определяем,
    # что соответствует флагам OFF переданным в ExternalProject_Add выше
endif()

if(IMGUI_AVAILABLE)
    target_link_libraries(RKeng PRIVATE rk_imgui)
    target_compile_definitions(RKeng PRIVATE RK_IMGUI_ENABLED)
endif()

if(ASSIMP_AVAILABLE)
    target_link_libraries(RKeng PRIVATE rk_assimp assimp)
    target_compile_definitions(RKeng PRIVATE RK_ASSIMP_ENABLED)
endif()

if(WIN32)
    target_link_libraries(RKeng PRIVATE ws2_32 winmm)
endif()

# Экспортируем ВСЕ символы из exe (включая Jolt) — иначе DLL-плагины тащат свою
# копию libJolt.a с отдельными .refptr-указателями, и InitJoltFromEngine не помогает.
if(WIN32 AND NOT MSVC)
    target_link_options(RKeng PRIVATE
        "-Wl,--out-implib,${CMAKE_BINARY_DIR}/libRKeng.dll.a"
        "-Wl,--export-all-symbols"
    )
endif()

# ============================================================
#  BUILD FLAGS
# ============================================================
if(MSVC)
    target_compile_options(RKeng PRIVATE
        $<$<CONFIG:Debug>:   /Od /Zi /W4>
        $<$<CONFIG:Release>: /O2 /W3>
    )
    target_compile_definitions(RKeng PRIVATE
        $<$<CONFIG:Debug>:   RK_DEBUG _CRT_SECURE_NO_WARNINGS>
        $<$<CONFIG:Release>: RK_RELEASE>
    )
else()
    target_compile_options(RKeng PRIVATE
        $<$<CONFIG:Debug>:   -g -O0 -Wall -Wextra>
        $<$<CONFIG:Release>: -O3 -march=native>
    )
    target_compile_definitions(RKeng PRIVATE
        $<$<CONFIG:Debug>:   RK_DEBUG>
        $<$<CONFIG:Release>: RK_RELEASE>
    )
endif()

# ============================================================
#  КОНФИГУРАЦИЯ
# ============================================================
message(STATUS "================================")
message(STATUS " RKeng configuration summary")
message(STATUS "  Vulkan    : ${Vulkan_VERSION}")
message(STATUS "  Jolt      : ${JOLT_AVAILABLE}")
message(STATUS "  ImGui     : ${IMGUI_AVAILABLE}")
message(STATUS "  Assimp    : ${ASSIMP_AVAILABLE}")
message(STATUS "================================")

# ============================================================
#  Копируем шейдеры в build директорию
# ============================================================
if(EXISTS ${CMAKE_SOURCE_DIR}/shaders)
    add_custom_command(TARGET RKeng POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/shaders
            $<TARGET_FILE_DIR:RKeng>/shaders
        COMMENT "Copying shaders to build directory"
    )
endif()