#pragma once
#include <IScenePlugin.h>
#include <EngineAPI.h>
#include "../../RKeng/src/core/SceneState.h"

#include <sol/sol.hpp>
#include <string>

namespace RKeng
{
    class LuaScenePlugin final : public IScenePlugin
    {
    public:
        explicit LuaScenePlugin(std::string scriptPath);
        ~LuaScenePlugin() override = default;

        void OnLoad  (SceneState& scene, const EngineAPI& api) override;
        void OnTick  (SceneState& scene, float dt)             override;
        void OnUnload(SceneState& scene)                       override;

        const char* GetName() const override { return "LuaScene"; }

    private:
        void BindAll();

        std::string      m_scriptPath;
        sol::state       m_lua;

        SceneState*      m_scene = nullptr;
        const EngineAPI* m_api   = nullptr;

        sol::protected_function m_fnOnLoad;
        sol::protected_function m_fnOnTick;
        sol::protected_function m_fnOnUnload;
    };
}
