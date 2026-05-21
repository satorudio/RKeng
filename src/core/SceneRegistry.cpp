#include "SceneRegistry.h"
#include "../utils/Logger.h"
#include <stdexcept>

namespace RKeng::SceneRegistry
{
    // Глобальный реестр — живёт до конца программы.
    // unordered_map безопасен для статической инициализации: static local гарантирует
    // порядок конструирования до первого вызова (C++11 §6.7).
    static std::unordered_map<std::string, LoadFn>& Registry()
    {
        static std::unordered_map<std::string, LoadFn> s_reg;
        return s_reg;
    }

    bool Register(const std::string& name, LoadFn fn)
    {
        Registry()[name] = std::move(fn);
        return true;
    }

    void Load(const std::string& name, SceneState& scene, PhysicsState& physics)
    {
        auto& reg = Registry();
        auto it = reg.find(name);
        if (it == reg.end())
        {
            std::string avail;
            for (auto& [k, _] : reg) avail += " " + k;
            throw std::runtime_error(
                "[SceneRegistry] Scene '" + name + "' not found. Available:" + avail);
        }
        Logger::Info("[SceneRegistry] Loading scene: " + name);
        it->second(scene, physics);
    }

    std::vector<std::string> ListScenes()
    {
        std::vector<std::string> out;
        out.reserve(Registry().size());
        for (auto& [name, _] : Registry())
            out.push_back(name);
        return out;
    }
}
