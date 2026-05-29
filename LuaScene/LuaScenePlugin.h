#pragma once
#include <IScenePlugin.h>
#include <EngineAPI.h>
#include <SceneState.h>
#include <PhysicsState.h>

#include <sol/sol.hpp>
#include <string>

namespace RKeng
{
    class LuaScenePlugin final : public IScenePlugin
    {
    public:
        explicit LuaScenePlugin(std::string scriptPath);
        ~LuaScenePlugin() override = default;

        void OnLoad  (SceneState& scene, PhysicsState& physics, const EngineAPI& api) override;
        void OnTick  (SceneState& scene, PhysicsState& physics, float dt)             override;
        void OnUnload(SceneState& scene, PhysicsState& physics)                       override;

        const char* GetName() const override { return "LuaScene"; }

    private:
        void BindAll();

        std::string      m_scriptPath;
        sol::state       m_lua;

        SceneState*      m_scene   = nullptr;
        PhysicsState*    m_physics = nullptr;
        const EngineAPI* m_api     = nullptr;

        sol::protected_function m_fnOnLoad;
        sol::protected_function m_fnOnTick;
        sol::protected_function m_fnOnUnload;
    };
}
