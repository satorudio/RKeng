#pragma once
#include "RKExport.h"
#include "SceneState.h"
#include "PhysicsState.h"
#include "EngineAPI.h"
#include <vector>

namespace RKeng::WorldGen
{
    // Зона грязи — AABB в XZ-плоскости
    struct MudZone
    {
        float minX, maxX;
        float minZ, maxZ;
        float depth = 1.0f;
    };

    struct WorldConfig
    {
        float worldHalfSize   = 600.0f;
        int   heightmapCols   = 48;
        int   heightmapRows   = 48;
        float cellSize        = 25.0f;
        float maxHeightVar    = 2.8f;
        float terrainSmooth   = 0.55f;

        int   numVoxelWalls   = 18;
        int   numMudZones     = 7;
        int   numRocks        = 80;
        int   numRamps        = 12;

        // Устаревшие алиасы для обратной совместимости
        // (старый код использовал worldSize / numSolidBlocks)
        float& worldSize      = worldHalfSize;   // алиас
        int&   numSolidBlocks = numRocks;         // алиас

        unsigned int seed = 42u;
    };

    struct WorldData
    {
        std::vector<MudZone> mudZones;
    };

    // Генерирует мир: рельеф, препятствия, зоны грязи.
    // Все физические тела создаются через api.
    RK_API WorldData Generate(SceneState& scene, PhysicsState& ph,
                               const EngineAPI& api,
                               const WorldConfig& cfg = {});

    // Очищает воксельные стены из scene (тела физики живут в PhysicsSystem).
    RK_API void Destroy(SceneState& scene, PhysicsState& ph);
}
