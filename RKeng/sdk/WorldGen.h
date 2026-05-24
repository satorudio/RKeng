#pragma once
#include "RKExport.h"
#include "SceneState.h"
#include "PhysicsState.h"
#include "EngineAPI.h"
#include <vector>

namespace RKeng::WorldGen
{
    // Зона грязи — AABB в XZ-плоскости (Y игнорируется, грязь на земле)
    struct MudZone
    {
        float minX, maxX;
        float minZ, maxZ;
        float depth = 1.0f;  // 0..1, влияет на силу буксовки
    };

    struct WorldConfig
    {
        float worldHalfSize   = 600.0f;   // ±м от центра мира
        int   heightmapCols   = 48;
        int   heightmapRows   = 48;
        float cellSize        = 25.0f;    // размер ячейки рельефа в метрах
        float maxHeightVar    = 2.8f;     // макс. неровность (м)
        float terrainSmooth   = 0.55f;    // 0=хаотичный, 1=гладкий

        int   numVoxelWalls   = 18;       // воксельных заборов-препятствий
        int   numMudZones     = 7;        // зон грязи
        int   numRocks        = 80;       // каменных глыб
        int   numRamps        = 12;       // трамплинов/уступов

        unsigned int seed     = 42u;
    };

    // Результат генерации — данные нужные для тика (зоны грязи и т.п.)
    struct WorldData
    {
        std::vector<MudZone> mudZones;
    };

    // Генерирует мир: рельеф, препятствия, зоны грязи.
    // Все физические тела создаются через api.SpawnStaticBoxRot.
    RK_API WorldData Generate(SceneState& scene, PhysicsState& ph,
                               const EngineAPI& api,
                               const WorldConfig& cfg = {});

    // Очищает воксельные стены из scene.
    // Физические тела мира уничтожаются при PhysicsSystem::~.
    RK_API void Destroy(SceneState& scene, PhysicsState& ph);
}
