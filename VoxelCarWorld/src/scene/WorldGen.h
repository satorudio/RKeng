#pragma once
// WorldGen.h — генерация открытого мира для RAM Power Wagon сцены.
//
// Мир состоит из:
//   • Неровный ландшафт — сетка боксов с варьируемой высотой (хайтмап)
//   • Воксельные стенки — случайные препятствия через scene.voxelWalls
//   • Зоны грязи — прямоугольные регионы, хранятся в MudZone[]
//     (координаты передаются в CarTick для детекции буксовки)
//   • Трамплины и скальные уступы
//
// Все физические тела создаются ТОЛЬКО через EngineAPI — никаких JPH:: вызовов.

#include "SceneState.h"
#include "PhysicsState.h"
#include "EngineAPI.h"
#include <array>
#include <vector>

namespace RKeng::WorldGen
{
    // Зона грязи — AABB в XZ плоскости (Y игнорируется, грязь на земле)
    struct MudZone
    {
        float minX, maxX;
        float minZ, maxZ;
        float depth = 1.0f;  // 0..1, влияет на силу буксовки
    };

    // Конфиг генерации
    struct WorldConfig
    {
        float worldHalfSize   = 600.0f;   // ±600 м от центра
        int   heightmapCols   = 48;        // колонок в хайтмапе
        int   heightmapRows   = 48;        // рядов
        float cellSize        = 25.0f;     // размер одной ячейки пола в метрах
        float maxHeightVar    = 2.8f;      // максимальная высота неровности (м)
        float terrainSmooth   = 0.55f;     // 0=хаотичный, 1=гладкий (билинейная интерп.)

        int   numVoxelWalls   = 18;        // воксельных заборов-препятствий
        int   numMudZones     = 7;         // зон грязи
        int   numRocks        = 80;        // каменных глыб (статик-боксы)
        int   numRamps        = 12;        // трамплинов/уступов

        unsigned int seed     = 42u;
    };

    // Результат генерации — список зон грязи, нужен CarTick для буксовки
    struct WorldData
    {
        std::vector<MudZone> mudZones;
    };

    // Генерирует мир; возвращает WorldData для последующего использования в тике.
    WorldData Generate(SceneState& scene, PhysicsState& ph,
                       const EngineAPI& api, const WorldConfig& cfg = {});

    // Освобождает статические тела мира (вызывается в OnUnload).
    // Сейчас тела принадлежат PhysicsSystem — при Shutdown они удалятся сами,
    // но явный Destroy нужен для корректного Reload без перезапуска движка.
    void Destroy(SceneState& scene, PhysicsState& ph);

}  // namespace RKeng::WorldGen
