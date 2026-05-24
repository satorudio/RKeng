// CarMesh.cpp — строит box-меш пикапа (один box = кузов, без вокселей).
// Формат вершины: pos[3] + color[3] + normal[3] = 9 floats.

#include "CarMesh.h"
#include <array>

namespace RKeng::CarMesh
{
    struct Face { int nx,ny,nz; int ax,ay,az; int bx,by,bz; };
    static constexpr std::array<Face,6> FACES = {{
        { 0, 0, 1,  1,0,0,  0,1,0 },  // +Z
        { 0, 0,-1, -1,0,0,  0,1,0 },  // -Z
        { 1, 0, 0,  0,0,-1, 0,1,0 },  // +X
        {-1, 0, 0,  0,0, 1, 0,1,0 },  // -X
        { 0, 1, 0,  1,0,0,  0,0,-1},  // +Y
        { 0,-1, 0,  1,0,0,  0,0, 1},  // -Y
    }};

    static void PushFace(std::vector<float>& V, std::vector<uint32_t>& I,
                         float cx,float cy,float cz,
                         float hx,float hy,float hz,
                         const Face& f, float r,float g,float b)
    {
        float nx=(float)f.nx, ny=(float)f.ny, nz=(float)f.nz;
        float ax=(float)f.ax*hx, ay=(float)f.ay*hy, az=(float)f.az*hz;
        float bx=(float)f.bx*hx, by=(float)f.by*hy, bz=(float)f.bz*hz;

        // Центр грани
        float ox = cx + nx*hx;
        float oy = cy + ny*hy;
        float oz = cz + nz*hz;

        // Простое диффузное освещение по нормали
        float light = 0.5f + 0.5f * (ny * 0.8f + nz * 0.15f + nx * 0.05f);
        if (light < 0.3f) light = 0.3f;
        if (light > 1.0f) light = 1.0f;

        float verts[4][3] = {
            { ox-ax-bx, oy-ay-by, oz-az-bz },
            { ox+ax-bx, oy+ay-by, oz+az-bz },
            { ox+ax+bx, oy+ay+by, oz+az+bz },
            { ox-ax+bx, oy-ay+by, oz-az+bz },
        };

        uint32_t base = (uint32_t)(V.size()/9);
        for (auto& p : verts) {
            V.push_back(p[0]); V.push_back(p[1]); V.push_back(p[2]);
            V.push_back(r*light); V.push_back(g*light); V.push_back(b*light);
            V.push_back(nx); V.push_back(ny); V.push_back(nz);
        }
        I.push_back(base+0); I.push_back(base+1); I.push_back(base+2);
        I.push_back(base+0); I.push_back(base+2); I.push_back(base+3);
    }

    void Build(CarState& car)
    {
        car.meshVertices.clear();
        car.meshIndices.clear();
        auto& V = car.meshVertices;
        auto& I = car.meshIndices;

        const auto& p = car.params;
        const float hx = p.halfW, hy = p.halfH, hz = p.halfL;

        // Основной кузов — тёмно-синий
        for (const auto& f : FACES)
            PushFace(V,I, 0,0,0, hx,hy,hz, f, 0.10f,0.20f,0.45f);

        // Крыша — немного меньше и выше
        {
            float rh = hy * 0.6f, rw = hx * 0.8f, rl = hz * 0.55f;
            float ry = hy + rh;
            for (const auto& f : FACES)
                PushFace(V,I, 0,ry,hz*0.05f, rw,rh,rl, f, 0.07f,0.14f,0.30f);
        }

        // 4 колеса — серые цилиндры-боксы
        {
            float wr = p.wheelRadius, ww = p.wheelWidth * 0.8f;
            float wxOff = hx + ww + 0.01f;
            float wzF   =  hz * 0.70f;
            float wzR   = -hz * 0.70f;
            float wyOff = -(hy + p.suspMaxLen * 0.5f);

            float positions[4][3] = {
                {-wxOff, wyOff, wzF},
                { wxOff, wyOff, wzF},
                {-wxOff, wyOff, wzR},
                { wxOff, wyOff, wzR},
            };
            for (auto& wp : positions)
                for (const auto& f : FACES)
                    PushFace(V,I, wp[0],wp[1],wp[2], ww,wr,wr, f, 0.15f,0.15f,0.15f);
        }

        car.meshDirty = true;
    }
}
