#pragma once
// WorldGen.h — процедурный мир для пикапа.
// Только через EngineAPI — никаких JPH:: вызовов.

#include "SceneState.h"
#include "PhysicsState.h"
#include "EngineAPI.h"
#include <vector>

namespace RKeng::WorldGen
{
    struct WorldConfig
    {
        float worldHalfSize = 500.0f;
        int   hmapCols      = 40;
        int   hmapRows      = 40;
        float cellSize      = 25.0f;
        float maxHeight     = 3.0f;
        float smooth        = 0.5f;
        int   numRocks      = 60;
        int   numRamps      = 10;
        unsigned int seed   = 42u;
    };

    void Generate(SceneState& scene, PhysicsState& ph,
                  const EngineAPI& api,
                  const WorldConfig& cfg = {});
}
