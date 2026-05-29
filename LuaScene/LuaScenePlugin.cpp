// LuaScenePlugin.cpp

#include "LuaScenePlugin.h"

#include <IScenePlugin.h>
#include <EngineAPI.h>
#include <SceneState.h>
#include <PhysicsState.h>
#include <Logger.h>
#include <WorldGen.h>
#include <PlayerMove.h>

#include <string>
#include <cstdlib>

namespace RKeng
{

#define RK_SAFE(where, ...)                                                \
    do {                                                                   \
        try { __VA_ARGS__; }                                               \
        catch (const std::exception& _ex) {                                \
            Logger::Error("[LuaScene] EXCEPTION in " where                 \
                          ": " + std::string(_ex.what()));                 \
        }                                                                  \
        catch (...) {                                                       \
            Logger::Error("[LuaScene] UNKNOWN EXCEPTION in " where);       \
        }                                                                  \
    } while(0)

#define RK_LUA_CHECK(result, where)                                        \
    do {                                                                   \
        if (!(result).valid()) {                                           \
            sol::error _e = (result);                                      \
            Logger::Error("[LuaScene] Lua error in " where                 \
                          ": " + std::string(_e.what()));                  \
        }                                                                  \
    } while (0)

LuaScenePlugin::LuaScenePlugin(std::string scriptPath)
    : m_scriptPath(std::move(scriptPath))
{
    m_lua.open_libraries(
        sol::lib::base, sol::lib::math,
        sol::lib::string, sol::lib::table,
        sol::lib::io, sol::lib::os);
}

void LuaScenePlugin::OnLoad(SceneState& scene, PhysicsState& physics, const EngineAPI& api)
{
    m_scene   = &scene;
    m_physics = &physics;
    m_api     = &api;

    RK_SAFE("BindAll", BindAll());

    auto load = m_lua.load_file(m_scriptPath);
    if (!load.valid()) {
        sol::error e = load;
        Logger::Error("[LuaScene] cannot load '" + m_scriptPath + "': " + e.what());
        return;
    }

    RK_SAFE("script exec", {
        auto exec = load();
        if (!exec.valid()) {
            sol::error e = exec;
            Logger::Error("[LuaScene] script exec error: " + std::string(e.what()));
        }
    });

    m_fnOnLoad   = m_lua["on_load"];
    m_fnOnTick   = m_lua["on_tick"];
    m_fnOnUnload = m_lua["on_unload"];

    if (m_fnOnLoad.valid()) {
        RK_SAFE("on_load", {
            auto r = m_fnOnLoad();
            RK_LUA_CHECK(r, "on_load");
        });
    }
}

void LuaScenePlugin::OnTick(SceneState&, PhysicsState&, float dt)
{
    if (!m_fnOnTick.valid()) return;
    RK_SAFE("on_tick", {
        auto r = m_fnOnTick(dt);
        RK_LUA_CHECK(r, "on_tick");
    });
}

void LuaScenePlugin::OnUnload(SceneState&, PhysicsState&)
{
    if (m_fnOnUnload.valid()) {
        RK_SAFE("on_unload", {
            auto r = m_fnOnUnload();
            RK_LUA_CHECK(r, "on_unload");
        });
    }
    m_scene = nullptr; m_physics = nullptr; m_api = nullptr;
}

void LuaScenePlugin::BindAll()
{
    sol::table eng = m_lua.create_named_table("Engine");
    eng["version"] = m_api->engineVersion;

    // ── Логгер ───────────────────────────────────────────────────────────────
    if (m_api->LogInfo)
        eng.set_function("log_info",  [this](const std::string& s){ m_api->LogInfo (s.c_str()); });
    if (m_api->LogWarn)
        eng.set_function("log_warn",  [this](const std::string& s){ m_api->LogWarn (s.c_str()); });
    if (m_api->LogError)
        eng.set_function("log_error", [this](const std::string& s){ m_api->LogError(s.c_str()); });

    // ── Мир ──────────────────────────────────────────────────────────────────
    eng.set_function("world_generate",
        [this](sol::optional<sol::table> t) {
            RK_SAFE("world_generate", {
                WorldGen::WorldConfig cfg;
                if (t) {
                    cfg.worldSize      = t->get_or("world_size",       cfg.worldSize);
                    cfg.numVoxelWalls  = t->get_or("num_voxel_walls",  cfg.numVoxelWalls);
                    cfg.numSolidBlocks = t->get_or("num_solid_blocks",  cfg.numSolidBlocks);
                    cfg.numRamps       = t->get_or("num_ramps",         cfg.numRamps);
                    cfg.seed           = (unsigned)t->get_or("seed",    (int)cfg.seed);
                }
                Logger::Info("[LuaScene] WorldGen::Generate...");
                WorldGen::Generate(*m_scene, *m_physics, cfg);
                Logger::Info("[LuaScene] WorldGen::Generate OK");
            });
        });

    eng.set_function("world_destroy",
        [this]() {
            RK_SAFE("world_destroy", WorldGen::Destroy(*m_scene, *m_physics));
        });

    // ── BVH ──────────────────────────────────────────────────────────────────
    if (m_api->engineVersion >= 7 && m_api->OptimizeBroadPhase)
        eng.set_function("optimize_broadphase",
            [this]() {
                RK_SAFE("optimize_broadphase", {
                    Logger::Info("[LuaScene] OptimizeBroadPhase...");
                    m_api->OptimizeBroadPhase(*m_physics);
                    Logger::Info("[LuaScene] OptimizeBroadPhase OK");
                });
            });

    // ── Персонаж ─────────────────────────────────────────────────────────────────
    if (m_api->CreateCharacter)
        eng.set_function("create_character",
            [this](sol::optional<float> sx, sol::optional<float> sy, sol::optional<float> sz) -> bool
            {
                bool ok = false;
                RK_SAFE("create_character", {
                    RK_CharacterDesc d{};
                    d.spawnX            = sx.value_or(0.f);
                    d.spawnY            = sy.value_or(2.f);
                    d.spawnZ            = sz.value_or(0.f);
                    d.capsuleHalfHeight = 0.9f;
                    d.capsuleRadius     = 0.35f;
                    d.maxSlopeAngleDeg  = 45.f;
                    ok = m_api->CreateCharacter(*m_physics, d);
                    Logger::Info("[LuaScene] CreateCharacter ok=" + std::to_string(ok));
                });
                return ok;
            });

    eng.set_function("player_move",
        [this]() {
            RK_SAFE("player_move", PlayerMove::Run(*m_scene, *m_physics));
        });

    // ── Камера ───────────────────────────────────────────────────────────────
    eng.set_function("set_third_person",
        [this](bool v) {
            RK_SAFE("set_third_person", {
                m_scene->thirdPersonCamera = v;
                if (v) m_scene->player.currentHeight = 0.f;
            });
        });

    // ── Низкоуровневая физика ─────────────────────────────────────────────────
    if (m_api->SpawnStaticBox)
        eng.set_function("spawn_static_box",
            [this](float cx, float cy, float cz,
                   float hx, float hy, float hz,
                   sol::optional<bool> sensor) -> uint32_t
            {
                uint32_t id = UINT32_MAX;
                RK_SAFE("spawn_static_box", {
                    RK_BoxBody b{};
                    b.position = {cx,cy,cz}; b.halfExtents = {hx,hy,hz};
                    b.isSensor = sensor.value_or(false);
                    id = m_api->SpawnStaticBox(*m_physics, b);
                });
                return id;
            });

    if (m_api->SpawnStaticBoxRot)
        eng.set_function("spawn_static_box_rot",
            [this](float cx, float cy, float cz,
                   float hx, float hy, float hz,
                   float ry, float rx) -> uint32_t
            {
                uint32_t id = UINT32_MAX;
                RK_SAFE("spawn_static_box_rot", {
                    RK_StaticBox b{};
                    b.cx=cx; b.cy=cy; b.cz=cz;
                    b.hx=hx; b.hy=hy; b.hz=hz;
                    b.rotY=ry; b.rotX=rx;
                    id = m_api->SpawnStaticBoxRot(*m_physics, b);
                });
                return id;
            });

    if (m_api->SpawnDynamicBox)
        eng.set_function("spawn_dynamic_box",
            [this](float cx, float cy, float cz,
                   float hx, float hy, float hz,
                   sol::optional<float> mass) -> uint32_t
            {
                uint32_t id = UINT32_MAX;
                RK_SAFE("spawn_dynamic_box", {
                    RK_DynamicBox b{};
                    b.cx=cx; b.cy=cy; b.cz=cz;
                    b.hx=hx; b.hy=hy; b.hz=hz;
                    b.mass = mass.value_or(100.f);
                    id = m_api->SpawnDynamicBox(*m_physics, b);
                });
                return id;
            });

    if (m_api->DestroyBody)
        eng.set_function("destroy_body",
            [this](uint32_t id) {
                RK_SAFE("destroy_body", m_api->DestroyBody(*m_physics, id));
            });

    if (m_api->GetBodyTransform)
        eng.set_function("get_body_transform",
            [this](uint32_t id)
                -> std::tuple<float,float,float,float,float,float,float>
            {
                float px=0,py=0,pz=0,qx=0,qy=0,qz=0,qw=1;
                RK_SAFE("get_body_transform",
                    m_api->GetBodyTransform(*m_physics,id,px,py,pz,qx,qy,qz,qw));
                return {px,py,pz,qx,qy,qz,qw};
            });

    // ── InputState ────────────────────────────────────────────────────────────
    m_lua.new_usertype<InputState>("InputState",
        "forward",      sol::readonly_property([](const InputState& s){ return s.forward; }),
        "backward",     sol::readonly_property([](const InputState& s){ return s.backward; }),
        "left",         sol::readonly_property([](const InputState& s){ return s.left; }),
        "right",        sol::readonly_property([](const InputState& s){ return s.right; }),
        "jump",         sol::readonly_property([](const InputState& s){ return s.jump; }),
        "jump_pressed", sol::readonly_property([](const InputState& s){ return s.jumpPressed; }),
        "crouch",       sol::readonly_property([](const InputState& s){ return s.crouch; }),
        "run",          sol::readonly_property([](const InputState& s){ return s.run; }),
        "mouse_dx",     sol::readonly_property([](const InputState& s){ return s.mouseDeltaX; }),
        "mouse_dy",     sol::readonly_property([](const InputState& s){ return s.mouseDeltaY; }),
        "yaw",          sol::readonly_property([](const InputState& s){ return s.yaw; }),
        "pitch",        sol::readonly_property([](const InputState& s){ return s.pitch; })
    );
    eng.set("input", std::ref(m_scene->input));

    Logger::Info("[LuaScene] BindAll OK (engineVersion=" +
                 std::to_string(m_api->engineVersion) + ")");
}

} // namespace RKeng

extern "C"
{
    RK_EXPORT RKeng::IScenePlugin* RK_CreateScene()
    {
        const char* env = std::getenv("RK_LUA_SCENE");
        return new RKeng::LuaScenePlugin(env ? env : "scene.lua");
    }
    RK_EXPORT void RK_DestroyScene(RKeng::IScenePlugin* p) { delete p; }
}
