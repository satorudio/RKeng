#include "CarMesh.h"
#include <glm/glm.hpp>
#include <array>

namespace RKeng::CarMesh
{
    // Вспомогательные направления и нормали 6 граней куба
    struct Face { int nx, ny, nz; int ax, ay, az; int bx, by, bz; };
    static constexpr std::array<Face,6> FACES = {{
        { 0, 0, 1,   1,0,0,  0,1,0 },  // +Z (перед)
        { 0, 0,-1,  -1,0,0,  0,1,0 },  // -Z (зад)
        { 1, 0, 0,   0,0,-1, 0,1,0 },  // +X (право)
        {-1, 0, 0,   0,0, 1, 0,1,0 },  // -X (лево)
        { 0, 1, 0,   1,0,0,  0,0,-1},  // +Y (верх)
        { 0,-1, 0,   1,0,0,  0,0, 1 }, // -Y (низ)
    }};

    static inline bool IsAlive(const CarState& car, int x, int y, int z)
    {
        if (x<0||x>=CAR_VOXELS_W) return false;
        if (y<0||y>=CAR_VOXELS_H) return false;
        if (z<0||z>=CAR_VOXELS_L) return false;
        return car.voxels[x][y][z].alive;
    }

    static void PushFace(std::vector<float>& verts, std::vector<uint32_t>& idxs,
                         float cx, float cy, float cz, float s,
                         const Face& f, const Vec3& col)
    {
        float hs = s * 0.5f;
        float nx = (float)f.nx, ny = (float)f.ny, nz = (float)f.nz;
        float ax = (float)f.ax * hs, ay = (float)f.ay * hs, az = (float)f.az * hs;
        float bx = (float)f.bx * hs, by = (float)f.by * hs, bz = (float)f.bz * hs;

        // Смещаем центр грани
        float ox = cx + nx * hs;
        float oy = cy + ny * hs;
        float oz = cz + nz * hs;

        // 4 вершины грани: (±a ± b)
        float positions[4][3] = {
            { ox - ax - bx, oy - ay - by, oz - az - bz },
            { ox + ax - bx, oy + ay - by, oz + az - bz },
            { ox + ax + bx, oy + ay + by, oz + az + bz },
            { ox - ax + bx, oy - ay + by, oz - az + bz },
        };

        // Небольшое затемнение по нормали для псевдо-освещения
        float light = 0.4f + 0.6f * (nx*0.3f + ny*1.0f + nz*0.2f) * 0.5f + 0.5f;
        light = glm::clamp(light, 0.3f, 1.0f);

        uint32_t base = (uint32_t)(verts.size() / 9);
        for (auto& p : positions)
        {
            verts.push_back(p[0]);
            verts.push_back(p[1]);
            verts.push_back(p[2]);
            verts.push_back(nx);
            verts.push_back(ny);
            verts.push_back(nz);
            verts.push_back(col.r * light);
            verts.push_back(col.g * light);
            verts.push_back(col.b * light);
        }
        // 2 треугольника
        idxs.push_back(base+0); idxs.push_back(base+1); idxs.push_back(base+2);
        idxs.push_back(base+0); idxs.push_back(base+2); idxs.push_back(base+3);
    }

    void Rebuild(CarState& car)
    {
        car.meshVertices.clear();
        car.meshIndices.clear();

        const float S = CAR_VOXEL_SIZE;
        // Начало координат в центре низа машины
        const float offX = -CAR_VOXELS_W * S * 0.5f;
        const float offY =  0.0f;
        const float offZ = -CAR_VOXELS_L * S * 0.5f;

        for (int x = 0; x < CAR_VOXELS_W; x++)
        for (int y = 0; y < CAR_VOXELS_H; y++)
        for (int z = 0; z < CAR_VOXELS_L; z++)
        {
            const auto& v = car.voxels[x][y][z];
            if (!v.alive) continue;

            float cx = offX + x * S + S * 0.5f;
            float cy = offY + y * S + S * 0.5f;
            float cz = offZ + z * S + S * 0.5f;

            // Немного темнее если здоровье низкое (повреждён)
            Vec3 col = v.color * (0.5f + 0.5f * v.health);

            for (int fi = 0; fi < 6; fi++)
            {
                const Face& f = FACES[fi];
                // Скрытые грани не рисуем (frustum culling на уровне вокселей)
                int nx = x + f.nx, ny = y + f.ny, nz = z + f.nz;
                if (IsAlive(car, nx, ny, nz)) continue;
                PushFace(car.meshVertices, car.meshIndices, cx, cy, cz, S, f, col);
            }
        }

        // Дебрис — добавляем к тому же мешу
        for (const auto& d : car.debris)
        {
            if (d.dead) continue;
            // Дебрис — просто один кубик
            float cx = d.pos.x - car.position.x; // локально
            float cy = d.pos.y - car.position.y;
            float cz = d.pos.z - car.position.z;
            for (int fi = 0; fi < 6; fi++)
                PushFace(car.meshVertices, car.meshIndices, cx, cy, cz, d.size, FACES[fi], d.color);
        }

        car.meshDirty = true;
    }
}
