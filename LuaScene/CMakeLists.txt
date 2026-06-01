cmake_minimum_required(VERSION 3.20)
project(LuaScene VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Пути к движку ────────────────────────────────────────────────────────────
if(NOT DEFINED RKENG_DIR)
    get_filename_component(RKENG_DIR "${CMAKE_SOURCE_DIR}/../RKeng" ABSOLUTE)
endif()
get_filename_component(RKENG_DIR "${RKENG_DIR}" ABSOLUTE)

set(RKENG_SDK_DIR "${RKENG_DIR}/engine_api")
set(RKENG_LIB_DIR "${RKENG_DIR}/lib")
set(RKENG_BUILD   "${RKENG_DIR}/build")

message(STATUS "[LuaScene] RKENG_DIR   = ${RKENG_DIR}")
message(STATUS "[LuaScene] RKENG_BUILD = ${RKENG_BUILD}")
message(STATUS "[LuaScene] RKENG_API   = ${RKENG_SDK_DIR}")

if(NOT EXISTS "${RKENG_SDK_DIR}/IScenePlugin.h")
    message(FATAL_ERROR
        "[LuaScene] engine_api не найден в '${RKENG_SDK_DIR}'.\n"
        "  Сначала собери RKengCore, или передай -DRKENG_DIR=<путь>")
endif()

# ── GLM (так же как в VoxelCarWorld) ─────────────────────────────────────────
add_library(LS_GLM INTERFACE)
target_include_directories(LS_GLM INTERFACE "${RKENG_LIB_DIR}/glm")
target_compile_definitions(LS_GLM INTERFACE
    GLM_FORCE_RADIANS
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_ENABLE_EXPERIMENTAL)

# ── RKengCore import library ─────────────────────────────────────────────────
set(RKENG_IMPLIB "${RKENG_BUILD}/libRKengCore.dll.a")
if(EXISTS "${RKENG_IMPLIB}")
    add_library(LS_RKengCore STATIC IMPORTED)
    set_target_properties(LS_RKengCore PROPERTIES IMPORTED_LOCATION "${RKENG_IMPLIB}")
    set(RKENG_IMP_OK TRUE)
    message(STATUS "[LuaScene] libRKengCore.dll.a found")
else()
    set(RKENG_IMP_OK FALSE)
    message(WARNING "[LuaScene] libRKengCore.dll.a не найден — Logger не слинкуется")
endif()

# ── Lua 5.4 ──────────────────────────────────────────────────────────────────
# Сначала ищем prebuilt в lib/lua, потом системный
set(LUA_DIR "${RKENG_LIB_DIR}/lua")
if(EXISTS "${LUA_DIR}/include/lua.h")
    add_library(LS_Lua STATIC IMPORTED)
    if(EXISTS "${LUA_DIR}/lib/liblua.a")
        set_target_properties(LS_Lua PROPERTIES IMPORTED_LOCATION "${LUA_DIR}/lib/liblua.a")
    elseif(EXISTS "${LUA_DIR}/liblua54.a")
        set_target_properties(LS_Lua PROPERTIES IMPORTED_LOCATION "${LUA_DIR}/liblua54.a")
    endif()
    target_include_directories(LS_Lua INTERFACE "${LUA_DIR}/include")
    set(LUA_OK TRUE)
    message(STATUS "[LuaScene] Lua: prebuilt at ${LUA_DIR}")
else()
    find_package(Lua 5.4 QUIET)
    if(LUA_FOUND)
        add_library(LS_Lua INTERFACE)
        target_include_directories(LS_Lua INTERFACE "${LUA_INCLUDE_DIR}")
        # LUA_LIBRARIES — путь к .a/.lib, передаём через переменную ниже
        set(LUA_LINK "${LUA_LIBRARIES}")
        set(LUA_OK TRUE)
        message(STATUS "[LuaScene] Lua: системный ${LUA_VERSION_STRING}")
    else()
        set(LUA_OK FALSE)
        message(FATAL_ERROR
            "[LuaScene] Lua 5.4 не найден.\n"
            "  Положи в ${LUA_DIR}/include/lua.h + lib/liblua.a\n"
            "  или установи системный Lua 5.4.")
    endif()
endif()

# ── sol2 (header-only) ───────────────────────────────────────────────────────
# Ищем в lib/sol2/include/sol/sol.hpp
set(SOL2_DIR "${RKENG_LIB_DIR}/sol2")
if(NOT EXISTS "${SOL2_DIR}/include/sol/sol.hpp")
    # Может быть просто распакован без вложенной папки include/
    if(EXISTS "${SOL2_DIR}/sol/sol.hpp")
        set(SOL2_INCLUDE "${SOL2_DIR}")
    else()
        message(FATAL_ERROR
            "[LuaScene] sol2 не найден в '${SOL2_DIR}'.\n"
            "  Скачай https://github.com/ThePhD/sol2/releases и распакуй в\n"
            "  ${SOL2_DIR}/  (чтобы был ${SOL2_DIR}/include/sol/sol.hpp)")
    endif()
else()
    set(SOL2_INCLUDE "${SOL2_DIR}/include")
endif()
message(STATUS "[LuaScene] sol2: ${SOL2_INCLUDE}")

add_library(LS_sol2 INTERFACE)
target_include_directories(LS_sol2 INTERFACE "${SOL2_INCLUDE}")

# ── Исходники ────────────────────────────────────────────────────────────────
add_library(LuaScene SHARED
    LuaScenePlugin.cpp
    LuaScenePlugin.h
)

target_include_directories(LuaScene PRIVATE
    "${CMAKE_SOURCE_DIR}"       # чтобы #include "LuaScenePlugin.h" работал
    "${RKENG_SDK_DIR}"          # IScenePlugin.h, EngineAPI.h
    "${RKENG_DIR}/src"          # SceneState.h, RKExport.h, Logger.h, ...
    "${RKENG_LIB_DIR}"          # на случай прямых lib-инклудов
)

target_compile_definitions(LuaScene PRIVATE
    RK_SCENE_BUILD
    SOL_ALL_SAFETIES_ON=1
    $<$<CONFIG:Debug>:   RK_DEBUG>
    $<$<CONFIG:Release>: RK_RELEASE>
    GLM_FORCE_RADIANS
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_ENABLE_EXPERIMENTAL
)

target_link_libraries(LuaScene PRIVATE
    LS_GLM
    LS_sol2
    LS_Lua
    $<$<BOOL:${LUA_LINK}>:${LUA_LINK}>
    $<$<BOOL:${RKENG_IMP_OK}>:LS_RKengCore>
)

if(WIN32)
    target_link_libraries(LuaScene PRIVATE ws2_32 winmm psapi)
endif()

# ── Флаги компилятора ─────────────────────────────────────────────────────────
if(MSVC)
    target_compile_options(LuaScene PRIVATE
        $<$<CONFIG:Debug>:   /Od /Zi /W4 /WX->
        $<$<CONFIG:Release>: /O2 /GL>)
    target_link_options(LuaScene PRIVATE
        $<$<CONFIG:Release>: /LTCG>)
else()
    target_compile_options(LuaScene PRIVATE
        -Wall -Wextra -Wno-unused-parameter
        $<$<CONFIG:Debug>:   -g -O0>
        $<$<CONFIG:Release>: -O3 -march=native>)
endif()

# ── Post-build: кидаем DLL и scene.lua в build движка ────────────────────────
add_custom_command(TARGET LuaScene POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:LuaScene>
        "${RKENG_BUILD}/$<TARGET_FILE_NAME:LuaScene>"
    COMMENT "[LuaScene] DLL → ${RKENG_BUILD}/"
)

if(EXISTS "${CMAKE_SOURCE_DIR}/scene.lua")
    add_custom_command(TARGET LuaScene POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/scene.lua"
            "${RKENG_BUILD}/scene.lua"
        COMMENT "[LuaScene] scene.lua → ${RKENG_BUILD}/"
    )
endif()

message(STATUS "[LuaScene] Configured OK")