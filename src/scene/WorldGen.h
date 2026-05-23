#pragma once
#include "../core/SceneState.h"
#include "../physics/PhysicsState.h"

namespace RKeng::WorldGen
{
    struct WorldConfig
    {
        float worldSize      = 200.0f;   // полуразмер мира (квадрат -worldSize..+worldSize)
        int   numVoxelWalls  = 30;       // воксельных стен рандомно по миру
        int   numSolidBlocks = 20;       // нерушимых бетонных блоков
        int   numRamps       = 8;        // трамплины
        unsigned int seed    = 42;
    };

    // Генерирует мир: пол, барьеры, рандомные препятствия
    void Generate(SceneState& scene, PhysicsState& ph, const WorldConfig& cfg = {});
}
