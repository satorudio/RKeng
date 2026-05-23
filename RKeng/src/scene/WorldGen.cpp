#include "WorldGen.h"
#include "../voxel/VoxelWall.h"
#include "../utils/Logger.h"
#include <cstdlib>
#include <cmath>
#include <string>

#ifdef RK_JOLT_ENABLED
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#endif

namespace RKeng::WorldGen
{
    // ----------------------------------------------------------------
    //  LCG PRNG — не трогает глобальный rand
    // ----------------------------------------------------------------
    struct Rng {
        uint32_t state;
        explicit Rng(uint32_t seed) : state(seed) {}
        uint32_t next() { state = state * 1664525u + 1013904223u; return state; }
        float nextf(float lo, float hi) {
            return lo + (hi - lo) * ((next() & 0xFFFF) / 65535.0f);
        }
        int nexti(int lo, int hi) { return lo + (int)(next() % (uint32_t)(hi - lo + 1)); }
    };

    // ----------------------------------------------------------------
    //  Создать статик-box в физике и сохранить BodyID
    // ----------------------------------------------------------------
#ifdef RK_JOLT_ENABLED
    static JPH::BodyID AddStaticBox(PhysicsState& ph,
                                    float cx, float cy, float cz,
                                    float hx, float hy, float hz,
                                    float rotY = 0.0f)
    {
        JPH::BoxShapeSettings ss(JPH::Vec3(hx, hy, hz));
        ss.SetEmbedded();

        // ИСПРАВЛЕНИЕ: сохраняем результат Create() в переменную.
        // Раньше было ss.Create().Get() — временный ShapeResult уничтожался
        // до вызова .Get(), что приводило к UB и потенциальному крашу.
        auto shapeResult = ss.Create();
        if (shapeResult.HasError()) {
            Logger::Error("WorldGen::AddStaticBox: BoxShape create error");
            return JPH::BodyID();
        }

        JPH::BodyCreationSettings bcs(
            shapeResult.Get(),
            JPH::RVec3(cx, cy, cz),
            JPH::Quat::sRotation(JPH::Vec3::sAxisY(), rotY),
            JPH::EMotionType::Static,
            PhysLayers::STATIC);
        return ph.bodyInterface->CreateAndAddBody(bcs, JPH::EActivation::DontActivate);
    }
#endif

    // Вспомогательная функция: добавить тело и сохранить ID в scene
#ifdef RK_JOLT_ENABLED
    static void AddStaticBoxTracked(SceneState& scene, PhysicsState& ph,
                                    float cx, float cy, float cz,
                                    float hx, float hy, float hz,
                                    float rotY = 0.0f)
    {
        auto id = AddStaticBox(ph, cx, cy, cz, hx, hy, hz, rotY);
        if (!id.IsInvalid())
            scene.worldStaticBodyIDs.push_back(id);
    }
#endif

    void Destroy(SceneState& scene, PhysicsState& ph)
    {
#ifdef RK_JOLT_ENABLED
        if (!ph.initialized || !ph.bodyInterface) return;

        // Удаляем статические тела мира (пол, стены, блоки, трамплины)
        for (auto id : scene.worldStaticBodyIDs)
        {
            if (!id.IsInvalid()) {
                ph.bodyInterface->RemoveBody(id);
                ph.bodyInterface->DestroyBody(id);
            }
        }
        scene.worldStaticBodyIDs.clear();

        // Удаляем физические тела воксельных стен
        for (auto& wall : scene.voxelWalls)
        {
            for (auto& bid : wall.voxelBodyIDs)
            {
                if (!bid.IsInvalid()) {
                    ph.bodyInterface->RemoveBody(bid);
                    ph.bodyInterface->DestroyBody(bid);
                    bid = JPH::BodyID();
                }
            }
        }
        scene.voxelWalls.clear();
#else
        (void)scene; (void)ph;
#endif
    }

    void Generate(SceneState& scene, PhysicsState& ph, const WorldConfig& cfg)
    {
        Rng rng(cfg.seed);

#ifdef RK_JOLT_ENABLED
        if (!ph.initialized) return;

        // Сбрасываем старые ID на случай повторной генерации
        scene.worldStaticBodyIDs.clear();

        // ---- 1. Большой пол ----------------------------------------
        //   WorldSize = 200 -> пол 400x400 м
        AddStaticBoxTracked(scene, ph,
            0.0f, -0.1f, 0.0f,
            cfg.worldSize, 0.1f, cfg.worldSize);

        // ---- 2. Граничные стены по периметру мира --------
        const float Wf = cfg.worldSize;
        float wallH = 5.0f;
        // Стены стоят НА краю мира, не внутри
        AddStaticBoxTracked(scene, ph,  Wf + 0.5f, wallH*0.5f,  0,    0.5f, wallH, Wf + 1.0f);  // +X
        AddStaticBoxTracked(scene, ph, -Wf - 0.5f, wallH*0.5f,  0,    0.5f, wallH, Wf + 1.0f);  // -X
        AddStaticBoxTracked(scene, ph,  0, wallH*0.5f,  Wf + 0.5f,    Wf + 1.0f, wallH, 0.5f);  // +Z
        AddStaticBoxTracked(scene, ph,  0, wallH*0.5f, -Wf - 0.5f,    Wf + 1.0f, wallH, 0.5f);  // -Z

        // ---- 3. Нерушимые бетонные блоки ----------------------------
        for (int i = 0; i < cfg.numSolidBlocks; i++)
        {
            float x    = rng.nextf(-Wf * 0.85f, Wf * 0.85f);
            float z    = rng.nextf(-Wf * 0.85f, Wf * 0.85f);
            float rotY = rng.nextf(0, 3.14159f);
            float hx   = rng.nextf(0.5f, 3.0f);
            float hy   = rng.nextf(0.5f, 2.0f);
            float hz   = rng.nextf(0.5f, 3.0f);

            if (std::abs(x) < 5.0f && std::abs(z) < 5.0f) continue;

            AddStaticBoxTracked(scene, ph, x, hy, z, hx, hy, hz, rotY);
        }

        // ---- 4. Трамплины -------------------------------------------
        for (int i = 0; i < cfg.numRamps; i++)
        {
            float x    = rng.nextf(-Wf * 0.7f, Wf * 0.7f);
            float z    = rng.nextf(-Wf * 0.7f, Wf * 0.7f);
            float rotY = rng.nextf(0, 3.14159f * 2.0f);

            if (std::abs(x) < 8.0f && std::abs(z) < 8.0f) continue;

            // Трамплин: наклонная плита
            float hx = 2.0f, hy = 0.15f, hz = 4.0f;
            // Наклон через ротацию X ~15°
            float tiltX = 0.26f; // ~15 градусов

            JPH::BoxShapeSettings ss(JPH::Vec3(hx, hy, hz));
            ss.SetEmbedded();

            // ИСПРАВЛЕНИЕ: то же что в AddStaticBox — сохраняем результат.
            auto shapeResult = ss.Create();
            if (shapeResult.HasError()) {
                Logger::Error("WorldGen: ramp BoxShape create error, skipping");
                continue;
            }

            JPH::BodyCreationSettings bcs(
                shapeResult.Get(),
                JPH::RVec3(x, 0.5f, z),
                JPH::Quat::sRotation(JPH::Vec3::sAxisY(), rotY) *
                JPH::Quat::sRotation(JPH::Vec3::sAxisX(), tiltX),
                JPH::EMotionType::Static,
                PhysLayers::STATIC);
            auto rampID = ph.bodyInterface->CreateAndAddBody(bcs, JPH::EActivation::DontActivate);
            if (!rampID.IsInvalid())
                scene.worldStaticBodyIDs.push_back(rampID);
        }

#endif

        // ---- 5. Воксельные стены ------------------------------------
        //   Рандомно по миру — это разрушаемые препятствия
        scene.voxelWalls.clear();
        scene.voxelWalls.reserve(cfg.numVoxelWalls);

        const float W = cfg.worldSize;
        uint32_t wallID = 0;
        for (int i = 0; i < cfg.numVoxelWalls; i++)
        {
            float x    = rng.nextf(-W * 0.8f, W * 0.8f);
            float z    = rng.nextf(-W * 0.8f, W * 0.8f);
            float rotY = rng.nextf(0.0f, 3.14159f);

            // Не спавним на старте
            if (std::abs(x) < 8.0f && std::abs(z) < 8.0f) continue;

            VoxelWall wall;
            wall.Init({ x, 0.0f, z }, rotY, wallID++);
            scene.voxelWalls.push_back(std::move(wall));
        }

#ifdef RK_JOLT_ENABLED
        // Физика для вокселей стен
        float H = VOXEL_SIZE * 0.5f;
        for (auto& wall : scene.voxelWalls)
        {
            for (int c = 0; c < VOXEL_COLS; c++)
            for (int r = 0; r < VOXEL_ROWS; r++)
            {
                Vec3 wp = wall.VoxelWorldPos(c, r);

                JPH::BoxShapeSettings vs(JPH::Vec3(H, H, H));
                vs.SetEmbedded();

                // ИСПРАВЛЕНИЕ: сохраняем результат Create() в переменную.
                auto voxelShapeResult = vs.Create();
                if (voxelShapeResult.HasError()) {
                    Logger::Error("WorldGen: voxel BoxShape create error, skipping");
                    continue;
                }

                JPH::BodyCreationSettings vbs(
                    voxelShapeResult.Get(),
                    JPH::RVec3(wp.x, wp.y, wp.z),
                    JPH::Quat::sIdentity(),
                    JPH::EMotionType::Static,
                    PhysLayers::STATIC);
                int idx = c * VOXEL_ROWS + r;
                wall.voxelBodyIDs[idx] = ph.bodyInterface->CreateAndAddBody(
                    vbs, JPH::EActivation::DontActivate);
            }
        }
#endif

        Logger::Info("WorldGen: generated world " +
                     std::to_string((int)(cfg.worldSize * 2)) + "x" +
                     std::to_string((int)(cfg.worldSize * 2)) + " m, " +
                     std::to_string(scene.voxelWalls.size()) + " voxel walls.");
    }
}