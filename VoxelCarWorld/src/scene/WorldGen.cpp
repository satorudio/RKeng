// WorldGen.cpp — генерирует рельеф, камни, рампы.
// Правило: ТОЛЬКО через EngineAPI. Никаких JPH::.

#include "WorldGen.h"
#include "VoxelWall.h"
#include <cmath>
#include <algorithm>
#include <string>

namespace RKeng::WorldGen
{
    struct Rng {
        uint32_t s;
        explicit Rng(uint32_t seed) : s(seed) {}
        uint32_t next() { s^=s<<13; s^=s>>17; s^=s<<5; return s; }
        float f(float lo, float hi) { return lo+(hi-lo)*((next()&0xFFFFu)/65535.f); }
        int   i(int lo, int hi)     { if(lo>=hi)return lo; return lo+(int)(next()%(unsigned)(hi-lo+1)); }
    };

    static void SpawnBox(const EngineAPI& api, PhysicsState& ph,
                         float cx, float cy, float cz,
                         float hx, float hy, float hz,
                         float rotY=0.f, float rotX=0.f)
    {
        RK_StaticBox b; b.cx=cx; b.cy=cy; b.cz=cz;
        b.hx=hx; b.hy=hy; b.hz=hz; b.rotY=rotY; b.rotX=rotX;
        api.SpawnStaticBoxRot(ph, b);
    }

    static void SpawnRamp(const EngineAPI& api, PhysicsState& ph,
                          float cx, float cz, float rotY,
                          float len, float width, float height)
    {
        float tiltX    = std::atan2(height, len);
        float hypo     = std::sqrt(len*len + height*height);
        float cosY     = std::cos(rotY), sinY = std::sin(rotY);
        SpawnBox(api,ph, cx+sinY*len*0.5f, height*0.5f, cz+cosY*len*0.5f,
                 width*0.5f, 0.2f, hypo*0.5f, rotY, -tiltX);
        // Верхняя площадка
        SpawnBox(api,ph, cx+sinY*(len+2.f), height, cz+cosY*(len+2.f),
                 width*0.5f, 0.2f, 2.f, rotY);
    }

    static std::vector<float> BuildHeightmap(int cols, int rows,
                                              float maxH, float smooth, uint32_t seed)
    {
        int nx=cols+1, nz=rows+1;
        std::vector<float> h(nx*nz);
        Rng r(seed);
        for (auto& v : h) v = r.f(-maxH, maxH);

        int passes = (int)(smooth * 8.f);
        for (int p=0; p<passes; p++) {
            std::vector<float> tmp = h;
            for (int iz=1;iz<nz-1;iz++)
            for (int ix=1;ix<nx-1;ix++) {
                float avg = (h[(iz-1)*nx+ix]+h[(iz+1)*nx+ix]+
                             h[iz*nx+(ix-1)]+h[iz*nx+(ix+1)]) * 0.25f;
                tmp[iz*nx+ix] = avg*0.7f + h[iz*nx+ix]*0.3f;
            }
            h = tmp;
        }
        // Центральная площадка — выравниваем
        int cx=nx/2, cz2=nz/2;
        for (int dz=-3;dz<=3;dz++) for (int dx=-3;dx<=3;dx++) {
            int ix=cx+dx, iz=cz2+dz;
            if (ix>=0&&ix<nx&&iz>=0&&iz<nz) {
                float fade = (float)std::max(std::abs(dx),std::abs(dz))/3.f;
                h[iz*nx+ix] *= fade;
            }
        }
        return h;
    }

    void Generate(SceneState& scene, PhysicsState& ph,
                  const EngineAPI& api, const WorldConfig& cfg)
    {
        scene.voxelWalls.clear();
        if (!ph.initialized)  { if(api.LogError) api.LogError("WorldGen: physics not init"); return; }
        if (!api.SpawnStaticBoxRot) { if(api.LogError) api.LogError("WorldGen: no SpawnStaticBoxRot"); return; }

        Rng rng(cfg.seed);
        const float W = cfg.worldHalfSize;

        // ── Хайтмап ───────────────────────────────────────────────────────
        auto hmap = BuildHeightmap(cfg.hmapCols, cfg.hmapRows,
                                   cfg.maxHeight, cfg.smooth,
                                   cfg.seed ^ 0xABCD1234u);
        {
            const int cols=cfg.hmapCols, rows=cfg.hmapRows;
            float sx = -(float)cols * cfg.cellSize * 0.5f;
            float sz = -(float)rows * cfg.cellSize * 0.5f;

            auto GetH = [&](int ix, int iz) {
                int cx=std::clamp(ix,0,cols), cz=std::clamp(iz,0,rows);
                return hmap[cz*(cols+1)+cx];
            };

            for (int iz=0;iz<rows;iz++) for (int ix=0;ix<cols;ix++) {
                float h00=GetH(ix,iz), h10=GetH(ix+1,iz);
                float h01=GetH(ix,iz+1), h11=GetH(ix+1,iz+1);
                float ch = (h00+h10+h01+h11)*0.25f;
                float dX = ((h10+h11)-(h00+h01))*0.5f;
                float dZ = ((h01+h11)-(h00+h10))*0.5f;
                float tX = std::atan2(dZ, cfg.cellSize);
                float tZ = std::atan2(dX, cfg.cellSize);
                float cx2 = sx + (ix+0.5f)*cfg.cellSize;
                float cz2 = sz + (iz+0.5f)*cfg.cellSize;
                float bh  = 3.0f + std::abs(ch);
                SpawnBox(api,ph, cx2, ch-bh*0.5f, cz2,
                         cfg.cellSize*0.5f, bh*0.5f, cfg.cellSize*0.5f, tZ, tX);
            }
        }

        // ── Периметр ──────────────────────────────────────────────────────
        {
            const float bW=3.f, bH=12.f;
            SpawnBox(api,ph,  W+bW, bH*.5f,  0.f, bW, bH, W+bW);
            SpawnBox(api,ph, -W-bW, bH*.5f,  0.f, bW, bH, W+bW);
            SpawnBox(api,ph,  0.f,  bH*.5f,  W+bW, W+bW, bH, bW);
            SpawnBox(api,ph,  0.f,  bH*.5f, -W-bW, W+bW, bH, bW);
        }

        // ── Камни ─────────────────────────────────────────────────────────
        for (int i=0; i<cfg.numRocks;) {
            float x=rng.f(-W*.88f,W*.88f), z=rng.f(-W*.88f,W*.88f);
            if (x*x+z*z < 200.f) continue; i++;
            float hx=rng.f(.5f,4.f), hy=rng.f(.4f,2.5f), hz=rng.f(.5f,4.f);
            float ry=rng.f(0.f,3.14159f), rx=rng.f(-.2f,.2f);
            SpawnBox(api,ph, x,hy,z, hx,hy,hz, ry,rx);
        }

        // ── Рампы ─────────────────────────────────────────────────────────
        for (int i=0; i<cfg.numRamps;) {
            float x=rng.f(-W*.75f,W*.75f), z=rng.f(-W*.75f,W*.75f);
            if (x*x+z*z < 400.f) continue; i++;
            SpawnRamp(api,ph, x,z,
                      rng.f(0.f,6.28f),
                      rng.f(8.f,18.f),
                      rng.f(5.f,10.f),
                      rng.f(1.f,4.f));
        }

        if (api.LogInfo) api.LogInfo("WorldGen: done");
    }
}
