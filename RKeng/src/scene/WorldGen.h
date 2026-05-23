#pragma once
#include "RKExport.h"
#include "../core/SceneState.h"
#include "../physics/PhysicsState.h"

namespace RKeng::WorldGen
{
    struct WorldConfig
    {
        float worldSize      = 200.0f;
        int   numVoxelWalls  = 30;
        int   numSolidBlocks = 20;
        int   numRamps       = 8;
        unsigned int seed    = 42;
    };

    RK_API void Generate(SceneState& scene, PhysicsState& ph, const WorldConfig& cfg = {});
    RK_API void Destroy(SceneState& scene, PhysicsState& ph);
}
