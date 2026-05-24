cmake_minimum_required(VERSION 3.20)
project(CubePhysicsScene VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Пути ────────────────────────────────────────────────────────────────────
if(NOT DEFINED RKENG_DIR)
    get_filename_component(RKENG_DIR "${CMAKE_SOURCE_DIR}/../RKeng" ABSOLUTE)
endif()
get_filename_component(RKENG_DIR "${RKENG_DIR}" ABSOLUTE)

set(RKENG_SDK_DIR  "${RKENG_DIR}/sdk")
set(RKENG_LIB_DIR  "${RKENG_DIR}/lib")
set(RKENG_BUILD    "${RKENG_DIR}/build")

message(STATUS "[CubePhysics] RKENG_DIR   = ${RKENG_DIR}")
message(STATUS "[CubePhysics] RKENG_BUILD = ${RKENG_BUILD}")
message(STATUS "[CubePhysics] RKENG_SDK   = ${RKENG_SDK_DIR}")

if(NOT EXISTS "${RKENG_SDK_DIR}/IScenePlugin.h")
    message(FATAL_ERROR
        "[CubePhysics] SDK not found at '${RKENG_SDK_DIR}'.\n"
        "  Pass: cmake -DRKENG_DIR=<absolute_path_to_RKeng> ..")
endif()

# ── Jolt ────────────────────────────────────────────────────────────────────
set(JOLT_DIR          "${RKENG_LIB_DIR}/JoltPhysics")
set(JOLT_LIB_PREBUILT "${JOLT_DIR}/libJolt.a")
set(JOLT_LIB_BUILT    "${RKENG_BUILD}/Jolt/libJolt.a")

if(EXISTS "${JOLT_LIB_PREBUILT}")
    set(JOLT_LIB "${JOLT_LIB_PREBUILT}")
elseif(EXISTS "${JOLT_LIB_BUILT}")
    set(JOLT_LIB "${JOLT_LIB_BUILT}")
else()
    set(JOLT_LIB "")
endif()

if(EXISTS "${JOLT_DIR}/Jolt/Jolt.h")
    if(JOLT_LIB)
        message(STATUS "[CubePhysics] Jolt lib: ${JOLT_LIB}")
        add_library(CP_Jolt STATIC IMPORTED)
        set_target_properties(CP_Jolt PROPERTIES
            IMPORTED_LOCATION "${JOLT_LIB}"
            INTERFACE_COMPILE_DEFINITIONS "JPH_OBJECT_LAYER_BITS=16;JPH_ENABLE_ASSERTS=0")
        target_include_directories(CP_Jolt INTERFACE "${JOLT_DIR}" "${JOLT_DIR}/..")
    else()
        add_library(CP_Jolt INTERFACE)
        target_include_directories(CP_Jolt INTERFACE "${JOLT_DIR}")
        target_compile_definitions(CP_Jolt INTERFACE JPH_OBJECT_LAYER_BITS=16 JPH_ENABLE_ASSERTS=0)
    endif()
    set(JOLT_OK TRUE)
else()
    message(WARNING "[CubePhysics] Jolt headers not found")
    set(JOLT_OK FALSE)
endif()

# ── RKengCore import library ────────────────────────────────────────────────
set(RKENG_IMPLIB "${RKENG_BUILD}/libRKengCore.dll.a")
if(EXISTS "${RKENG_IMPLIB}")
    message(STATUS "[CubePhysics] libRKengCore.dll.a: ${RKENG_IMPLIB}")
    add_library(RKengCore STATIC IMPORTED)
    set_target_properties(RKengCore PROPERTIES IMPORTED_LOCATION "${RKENG_IMPLIB}")
    set(RKENG_IMP_OK TRUE)
else()
    message(WARNING "[CubePhysics] libRKengCore.dll.a not found at ${RKENG_IMPLIB}")
    set(RKENG_IMP_OK FALSE)
endif()

# ── GLM ─────────────────────────────────────────────────────────────────────
add_library(CP_GLM INTERFACE)
target_include_directories(CP_GLM INTERFACE "${RKENG_LIB_DIR}/glm")
target_compile_definitions(CP_GLM INTERFACE
    GLM_FORCE_RADIANS
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_ENABLE_EXPERIMENTAL)

# ── Vulkan ──────────────────────────────────────────────────────────────────
find_package(Vulkan QUIET)

# GLFW: DLL-сцена не создаёт окон — линковать не нужно.
# GLFWwindow* приходит из движка как forward declaration.

# ── Исходники ───────────────────────────────────────────────────────────────
# Только наш главный файл — WorldGen, PlayerMove и прочее уже в RKengCore.dll
file(GLOB_RECURSE SCENE_SOURCES
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

message(STATUS "[CubePhysics] Sources:")
foreach(f ${SCENE_SOURCES})
    message(STATUS "   ${f}")
endforeach()

# ── DLL ─────────────────────────────────────────────────────────────────────
add_library(CubePhysicsScene SHARED ${SCENE_SOURCES})

target_include_directories(CubePhysicsScene PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${RKENG_SDK_DIR}"
    "${RKENG_LIB_DIR}"
    $<$<BOOL:${Vulkan_FOUND}>:${Vulkan_INCLUDE_DIRS}>
)

target_compile_definitions(CubePhysicsScene PRIVATE
    RK_SCENE_BUILD
    $<$<BOOL:${JOLT_OK}>:RK_JOLT_ENABLED>
    $<$<CONFIG:Debug>:   RK_DEBUG>
    $<$<CONFIG:Release>: RK_RELEASE>
    GLM_FORCE_RADIANS
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_ENABLE_EXPERIMENTAL
)

# ── GLFW ────────────────────────────────────────────────────────────────────
# DLL-сцена вызывает glfwGetKey/glfwGetCursorPos напрямую — нужна линковка.
find_library(GLFW_LIB glfw3 HINTS "${RKENG_LIB_DIR}" "${RKENG_LIB_DIR}/glfw/lib-mingw-w64" "C:/msys64/mingw64/lib")
if(NOT GLFW_LIB)
    find_package(glfw3 QUIET)
    if(glfw3_FOUND)
        set(GLFW_LIB glfw)
    else()
        message(WARNING "[CubePhysics] GLFW not found — input won't link")
        set(GLFW_LIB "")
    endif()
endif()
if(GLFW_LIB)
    message(STATUS "[CubePhysics] GLFW: ${GLFW_LIB}")
endif()

target_link_libraries(CubePhysicsScene PRIVATE
    CP_GLM
    $<$<BOOL:${Vulkan_FOUND}>:Vulkan::Vulkan>
    $<$<BOOL:${JOLT_OK}>:CP_Jolt>
    $<$<BOOL:${RKENG_IMP_OK}>:RKengCore>
    $<$<BOOL:${GLFW_LIB}>:${GLFW_LIB}>
)

if(WIN32)
    target_link_libraries(CubePhysicsScene PRIVATE ws2_32 winmm psapi)
endif()

# ── Компилятор ──────────────────────────────────────────────────────────────
if(MSVC)
    target_compile_options(CubePhysicsScene PRIVATE
        $<$<CONFIG:Debug>:   /Od /Zi /W4 /WX->
        $<$<CONFIG:Release>: /O2 /GL>)
    target_link_options(CubePhysicsScene PRIVATE
        $<$<CONFIG:Release>: /LTCG>)
else()
    target_compile_options(CubePhysicsScene PRIVATE
        -Wall -Wextra -Wno-unused-parameter
        $<$<CONFIG:Debug>:   -g -O0>
        $<$<CONFIG:Release>: -O3 -march=native -flto>)
    target_link_options(CubePhysicsScene PRIVATE
        $<$<CONFIG:Release>: -flto>)
endif()

# ── Post-build: копируем DLL в build движка ─────────────────────────────────
add_custom_command(TARGET CubePhysicsScene POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:CubePhysicsScene>
        "${RKENG_BUILD}/$<TARGET_FILE_NAME:CubePhysicsScene>"
    COMMENT "[CubePhysics] Copied DLL → ${RKENG_BUILD}/"
)

message(STATUS "[CubePhysics] Configured OK. SDK: ${RKENG_SDK_DIR}")
