// WorldGen.cpp — генератор открытого мира (RAM Power Wagon сцена).
//
// АРХИТЕКТУРНЫЕ ПРАВИЛА (строго):
//   • Никаких JPH:: вызовов — все тела через EngineAPI
//   • Никакого #include <Jolt/...> — не нужен, типы только из PhysicsState
//   • Линковка только через libRKengCore.dll.a

#include "WorldGen.h"
#include "VoxelWall.h"

#include <cmath>
#include <string>
#include <algorithm>

namespace RKeng::WorldGen
{
    // ── Простой LCG-генератор ─────────────────────────────────────────────
    struct Rng
    {
        uint32_t s;
        explicit Rng(uint32_t seed) : s(seed) {}

        uint32_t next()
        {
            s ^= s << 13; s ^= s >> 17; s ^= s << 5;
            return s;
        }

        float f(float lo, float hi)
        {
            return lo + (hi - lo) * ((next() & 0xFFFFu) / 65535.0f);
        }

        int i(int lo, int hi)
        {
            if (lo >= hi) return lo;
            return lo + (int)(next() % (unsigned)(hi - lo + 1));
        }
    };

    // ── Вспомогательный SpawnStaticBox ────────────────────────────────────
    // Обёртка: принимает центр, half-extents, поворот Y и X
    static void SpawnBox(const EngineAPI& api, PhysicsState& ph,
                         float cx, float cy, float cz,
                         float hx, float hy, float hz,
                         float rotY = 0.0f, float rotX = 0.0f)
    {
        RK_StaticBox b;
        b.cx = cx; b.cy = cy; b.cz = cz;
        b.hx = hx; b.hy = hy; b.hz = hz;
        b.rotY = rotY;
        b.rotX = rotX;
        api.SpawnStaticBoxRot(ph, b);
    }

    // ── Трамплин ──────────────────────────────────────────────────────────
    // Наклонная плита + горизонтальная верхняя площадка
    static void SpawnRamp(const EngineAPI& api, PhysicsState& ph,
                          float cx, float cz, float rotY,
                          float rampLen, float rampWidth, float rampHeight)
    {
        // Угол подъёма и длина гипотенузы
        float tiltX       = std::atan2(rampHeight, rampLen);
        float hypoLen     = std::sqrt(rampLen * rampLen + rampHeight * rampHeight);
        float halfHypo    = hypoLen * 0.5f;
        float plateCY     = rampHeight * 0.5f;

        float cosY = std::cos(rotY), sinY = std::sin(rotY);

        // Наклонная плита
        SpawnBox(api, ph,
            cx + sinY * rampLen * 0.5f,
            plateCY,
            cz + cosY * rampLen * 0.5f,
            rampWidth * 0.5f, 0.20f, halfHypo,
            rotY, -tiltX);

        // Верхняя горизонтальная площадка
        SpawnBox(api, ph,
            cx + sinY * (rampLen + 2.0f),
            rampHeight,
            cz + cosY * (rampLen + 2.0f),
            rampWidth * 0.5f, 0.20f, 2.0f,
            rotY);
    }

    // ── Воксельная стена (препятствие) ────────────────────────────────────
    static void SpawnVoxelWall(SceneState& scene, PhysicsState& ph,
                               const EngineAPI& api,
                               float cx, float cz, float rotY)
    {
        // Один прямоугольный забор из вокселей
        // Используем статик-боксы через EngineAPI для физики,
        // и VoxelWall в SceneState для рендера
        VoxelWall wall;
        wall.origin = Vec3(cx, 0.0f, cz);
        wall.rotY   = rotY;

        for (int col = 0; col < VOXEL_COLS; col++)
        for (int row = 0; row < VOXEL_ROWS; row++)
            wall.alive[col][row] = true;

        wall.meshDirty = true;
        scene.voxelWalls.push_back(std::move(wall));

        // Физика — единый статик-бокс под весь забор
        float wallWidth  = VOXEL_COLS * VOXEL_SIZE;
        float wallHeight = VOXEL_ROWS * VOXEL_SIZE;
        float thickness  = VOXEL_SIZE * 0.5f;

        SpawnBox(api, ph,
            cx, wallHeight * 0.5f, cz,
            wallWidth * 0.5f, wallHeight * 0.5f, thickness,
            rotY);
    }

    // ── Хайтмап ───────────────────────────────────────────────────────────
    // Хранит высоту каждого узла сетки (в метрах над нулём).
    // Размер: (cols+1) × (rows+1) узлов.
    static std::vector<float> BuildHeightmap(int cols, int rows,
                                             float maxH, float smooth,
                                             uint32_t seed)
    {
        int nodesX = cols + 1;
        int nodesZ = rows + 1;
        std::vector<float> h(nodesX * nodesZ, 0.0f);

        // Белый шум
        Rng rng(seed);
        for (auto& v : h)
            v = rng.f(-maxH, maxH);

        // Многократное сглаживание (простая blur-like итерация)
        int passes = (int)(smooth * 8.0f);
        for (int p = 0; p < passes; p++)
        {
            std::vector<float> tmp = h;
            for (int iz = 1; iz < nodesZ - 1; iz++)
            for (int ix = 1; ix < nodesX - 1; ix++)
            {
                float avg = 0.0f;
                avg += h[(iz-1)*nodesX + ix];
                avg += h[(iz+1)*nodesX + ix];
                avg += h[iz*nodesX + (ix-1)];
                avg += h[iz*nodesX + (ix+1)];
                avg *= 0.25f;
                tmp[iz*nodesX + ix] = avg * 0.7f + h[iz*nodesX + ix] * 0.3f;
            }
            h = tmp;
        }

        // Центральная площадка (старт) — принудительно выравниваем
        // примерно 5×5 ячеек от центра (node nodesX/2, nodesZ/2)
        int cx = nodesX / 2;
        int cz = nodesZ / 2;
        for (int dz = -3; dz <= 3; dz++)
        for (int dx = -3; dx <= 3; dx++)
        {
            int ix = cx + dx, iz = cz + dz;
            if (ix >= 0 && ix < nodesX && iz >= 0 && iz < nodesZ)
            {
                float fade = std::max(std::abs(dx), std::abs(dz)) / 3.0f;
                h[iz*nodesX + ix] *= fade;
            }
        }

        return h;
    }

    // ── Генерация ─────────────────────────────────────────────────────────
    WorldData Generate(SceneState& scene, PhysicsState& ph,
                       const EngineAPI& api, const WorldConfig& cfg)
    {
        WorldData result;
        scene.voxelWalls.clear();

        if (!ph.initialized)
        {
            if (api.LogError) api.LogError("WorldGen: physics not initialized");
            return result;
        }
        if (!api.SpawnStaticBoxRot)
        {
            if (api.LogError) api.LogError("WorldGen: SpawnStaticBoxRot not bound");
            return result;
        }

        Rng rng(cfg.seed);

        const float W    = cfg.worldHalfSize;
        const float cell = cfg.cellSize;

        // ── 1. Хайтмап ────────────────────────────────────────────────────
        // Хранит высоту центров ячеек (не узлов) — для простоты
        const int cols = cfg.heightmapCols;
        const int rows = cfg.heightmapRows;

        auto hmap = BuildHeightmap(cols, rows,
                                   cfg.maxHeightVar, cfg.terrainSmooth,
                                   cfg.seed ^ 0xABCD1234u);

        // Генерируем физические боксы под каждую ячейку хайтмапа
        // Каждая ячейка — тонкая плита, поднятая/опущенная на нужную высоту
        // и слегка наклонённая для плавности
        {
            float startX = -(float)(cols) * cell * 0.5f;
            float startZ = -(float)(rows) * cell * 0.5f;

            auto GetH = [&](int ix, int iz) -> float {
                int cx = std::clamp(ix, 0, cols);
                int cz = std::clamp(iz, 0, rows);
                return hmap[cz * (cols + 1) + cx];
            };

            for (int iz = 0; iz < rows; iz++)
            for (int ix = 0; ix < cols; ix++)
            {
                // Средняя высота ячейки из 4 углов
                float h00 = GetH(ix,   iz);
                float h10 = GetH(ix+1, iz);
                float h01 = GetH(ix,   iz+1);
                float h11 = GetH(ix+1, iz+1);
                float centerH = (h00 + h10 + h01 + h11) * 0.25f;

                // Наклон по X — разница высот по оси X
                float dX = ((h10 + h11) - (h00 + h01)) * 0.5f;
                float dZ = ((h01 + h11) - (h00 + h10)) * 0.5f;

                float tiltX = std::atan2(dZ, cell);
                float tiltZ = std::atan2(dX, cell);

                // Центр ячейки
                float cx = startX + (ix + 0.5f) * cell;
                float cz = startZ + (iz + 0.5f) * cell;

                // Толщина плиты — чтобы снизу не было дыр
                // Выбираем достаточной толщины (max 5м) под нижней точкой
                float boxH = 2.5f + std::abs(centerH);

                SpawnBox(api, ph,
                    cx, centerH - boxH * 0.5f, cz,
                    cell * 0.5f, boxH * 0.5f, cell * 0.5f,
                    tiltZ, tiltX);  // небольшой наклон = более реалистичный рельеф
            }
        }

        // ── 2. Ограждение по периметру мира ──────────────────────────────
        {
            const float bW = 3.0f, bH = 10.0f;
            SpawnBox(api, ph,  W + bW, bH * 0.5f,  0.0f,  bW, bH, W + bW);
            SpawnBox(api, ph, -W - bW, bH * 0.5f,  0.0f,  bW, bH, W + bW);
            SpawnBox(api, ph,  0.0f,  bH * 0.5f,  W + bW, W + bW, bH, bW);
            SpawnBox(api, ph,  0.0f,  bH * 0.5f, -W - bW, W + bW, bH, bW);
        }

        // ── 3. Каменные глыбы / завалы ────────────────────────────────────
        for (int i = 0; i < cfg.numRocks; i++)
        {
            float x = rng.f(-W * 0.88f, W * 0.88f);
            float z = rng.f(-W * 0.88f, W * 0.88f);

            // Не ставим слишком близко к старту
            if (x * x + z * z < 225.0f) { i--; continue; }

            float hx = rng.f(0.5f, 4.5f);
            float hy = rng.f(0.4f, 3.0f);
            float hz = rng.f(0.5f, 4.5f);
            float ry = rng.f(0.0f, 3.14159f);
            float rx = rng.f(-0.25f, 0.25f);  // слегка повалены

            SpawnBox(api, ph,
                x, hy, z,
                hx, hy, hz,
                ry, rx);
        }

        // ── 4. Трамплины / скальные уступы ───────────────────────────────
        for (int i = 0; i < cfg.numRamps; i++)
        {
            float x = rng.f(-W * 0.75f, W * 0.75f);
            float z = rng.f(-W * 0.75f, W * 0.75f);
            if (x * x + z * z < 400.0f) { i--; continue; }

            float rotY   = rng.f(0.0f, 6.28318f);
            float rLen   = rng.f(8.0f, 20.0f);
            float rWidth = rng.f(5.0f, 10.0f);
            float rH     = rng.f(1.0f, 4.5f);

            SpawnRamp(api, ph, x, z, rotY, rLen, rWidth, rH);
        }

        // ── 5. Воксельные стены (препятствия) ────────────────────────────
        for (int i = 0; i < cfg.numVoxelWalls; i++)
        {
            float x = rng.f(-W * 0.70f, W * 0.70f);
            float z = rng.f(-W * 0.70f, W * 0.70f);
            if (x * x + z * z < 300.0f) { i--; continue; }

            float rotY = rng.f(0.0f, 6.28318f);
            SpawnVoxelWall(scene, ph, api, x, z, rotY);
        }

        // ── 6. Зоны грязи ─────────────────────────────────────────────────
        // Физика: не нужна (машина не тонет, только меняется фрикция).
        // Просто сохраняем AABB'ы в WorldData — CarTick проверяет точку колеса.
        for (int i = 0; i < cfg.numMudZones; i++)
        {
            float cx = rng.f(-W * 0.65f, W * 0.65f);
            float cz = rng.f(-W * 0.65f, W * 0.65f);
            if (cx * cx + cz * cz < 350.0f) { i--; continue; }

            float hw = rng.f(15.0f, 50.0f);
            float hl = rng.f(15.0f, 50.0f);

            MudZone zone;
            zone.minX  = cx - hw;  zone.maxX = cx + hw;
            zone.minZ  = cz - hl;  zone.maxZ = cz + hl;
            zone.depth = rng.f(0.5f, 1.0f);
            result.mudZones.push_back(zone);

            // Визуальная плита (тёмно-коричневый коллайдер на уровне земли)
            // Высота = 0.05 м — тонкая, просто чтобы обозначить зону
            // Цвет передаётся в рендер через voxels/mesh, физически это просто пол
            // Ставим один тонкий статик-бокс — для "вдавленности"
            SpawnBox(api, ph,
                cx, -0.08f, cz,
                hw, 0.08f, hl);
        }

        if (api.LogInfo)
        {
            std::string msg = "WorldGen: " +
                std::to_string(cols) + "x" + std::to_string(rows) +
                " terrain, " + std::to_string(cfg.numRocks) + " rocks, " +
                std::to_string(cfg.numVoxelWalls) + " voxel walls, " +
                std::to_string(result.mudZones.size()) + " mud zones.";
            api.LogInfo(msg.c_str());
        }

        return result;
    }

}  // namespace RKeng::WorldGen
