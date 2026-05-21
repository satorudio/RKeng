#include "VoxelWall.h"
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace RKeng
{
    // ----------------------------------------------------------------
    // Инициализация
    // ----------------------------------------------------------------
    void VoxelWall::Init(Vec3 pos, float rotationY, uint32_t wallID)
    {
        origin = pos;
        rotY   = rotationY;
        id     = wallID;

        for (int c = 0; c < VOXEL_COLS; c++)
            for (int r = 0; r < VOXEL_ROWS; r++)
                alive[c][r] = true;

        fallingVoxels.clear();

#ifdef RK_JOLT_ENABLED
        for (auto& bid : voxelBodyIDs)
            bid = JPH::BodyID();
#endif

        RebuildMesh();
    }

    // ----------------------------------------------------------------
    // Мировая позиция вокселя
    // ----------------------------------------------------------------
    Vec3 VoxelWall::VoxelWorldPos(int col, int row) const
    {
        float lx = (col - VOXEL_COLS * 0.5f + 0.5f) * VOXEL_SIZE;
        float ly = (row + 0.5f) * VOXEL_SIZE;
        float lz = 0.0f;

        float rad = glm::radians(rotY);
        float wx  = lx * cosf(rad) - lz * sinf(rad);
        float wz  = lx * sinf(rad) + lz * cosf(rad);

        return Vec3(origin.x + wx, origin.y + ly, origin.z + wz);
    }

    // ----------------------------------------------------------------
    // Цвет вокселя (публичный метод — нужен при создании FallingVoxel)
    // ----------------------------------------------------------------
    Vec3 VoxelWall::GetVoxelColor(int col, int row) const
    {
        float base = 0.55f + 0.15f * ((col * 7 + row * 13) % 7) / 7.0f;
        float worn = 0.9f - 0.04f * row;
        return Vec3(base * worn, base * 0.85f * worn, base * 0.75f * worn);
    }

    // ----------------------------------------------------------------
    // Разрушение одного вокселя — порождает FallingVoxel
    // ----------------------------------------------------------------
    bool VoxelWall::DestroyVoxel(int col, int row, Vec3 impulse)
    {
        if (col < 0 || col >= VOXEL_COLS) return false;
        if (row < 0 || row >= VOXEL_ROWS) return false;
        if (!alive[col][row]) return false;

        alive[col][row] = false;
        meshDirty = true;

        // Создаём падающий воксель
        FallingVoxel fv;
        fv.pos      = VoxelWorldPos(col, row);
        fv.size     = VOXEL_SIZE;
        fv.lifetime = 0.0f;
        fv.dead     = false;

        Vec3 c = GetVoxelColor(col, row);
        fv.color = c;

        // Небольшой случайный разброс — чтобы вокселя не летели ровно одинаково
        float jitter = VOXEL_SIZE * 1.5f;  // ~±0.375 м — аккуратный разброс
        auto h = [](int a, int b) -> float {
            int v = (a * 1973 + b * 9277) ^ (a * 4567);
            return ((v & 0xFFFF) / 65535.0f) * 2.0f - 1.0f;
        };
        // Импульс уже содержит нужное направление — добавляем небольшой jitter
        fv.velocity.x = impulse.x + h(col + (int)id * 100, row)     * jitter;
        fv.velocity.y = impulse.y + h(col, row + 77) * 1.5f + 2.0f; // вертикальный разброс ±, +2 базовый вылет
        fv.velocity.z = impulse.z + h(col + 13, row + (int)id * 50) * jitter;

        fallingVoxels.push_back(fv);

        RebuildMesh();
        return true;
    }

    // ----------------------------------------------------------------
    // Взрыв — разрушить все воксели в радиусе
    // ----------------------------------------------------------------
    bool VoxelWall::DestroyRadius(Vec3 worldHitPos, float radius)
    {
        bool changed = false;
        for (int c = 0; c < VOXEL_COLS; c++)
        {
            for (int r = 0; r < VOXEL_ROWS; r++)
            {
                if (!alive[c][r]) continue;
                Vec3 vp = VoxelWorldPos(c, r);
                float dx = vp.x - worldHitPos.x;
                float dy = vp.y - worldHitPos.y;
                float dz = vp.z - worldHitPos.z;
                float dist = sqrtf(dx*dx + dy*dy + dz*dz);
                if (dist <= radius)
                {
                    // Импульс — от центра взрыва
                    Vec3 impulse{0,0,0};
                    if (dist > 0.001f) {
                        float str = (1.0f - dist / radius) * 4.0f;
                        impulse.x = (dx / dist) * str;
                        impulse.y = (dy / dist) * str + 1.0f;
                        impulse.z = (dz / dist) * str;
                    }
                    DestroyVoxel(c, r, impulse);
                    changed = true;
                }
            }
        }
        return changed;
    }

    // ----------------------------------------------------------------
    // Симуляция падающих вокселей
    // ----------------------------------------------------------------
    void VoxelWall::UpdateFalling(float dt)
    {
        if (fallingVoxels.empty()) return;

        constexpr float GRAVITY      = -18.0f;
        constexpr float BOUNCE_DAMP  =  0.35f;   // коэф. отскока
        constexpr float FRICTION     =  0.75f;   // замедление по горизонтали при отскоке
        constexpr float GROUND_Y     =  0.0f;    // уровень пола
        constexpr float MAX_LIFETIME = 4.0f;     // после этого исчезают



        for (auto& fv : fallingVoxels)
        {
            if (fv.dead) continue;

            fv.lifetime += dt;
            if (fv.lifetime > MAX_LIFETIME) { fv.dead = true; continue; }

            // Гравитация
            fv.velocity.y += GRAVITY * dt;

            // Движение
            fv.pos.x += fv.velocity.x * dt;
            fv.pos.y += fv.velocity.y * dt;
            fv.pos.z += fv.velocity.z * dt;

            // Отскок от пола
            float bottom = fv.pos.y - fv.size * 0.5f;
            if (bottom < GROUND_Y)
            {
                fv.pos.y      = GROUND_Y + fv.size * 0.5f;
                fv.velocity.y = -fv.velocity.y * BOUNCE_DAMP;
                fv.velocity.x *= FRICTION;
                fv.velocity.z *= FRICTION;

                // Порог остановки — если отскок слабее 1 м/с, лежим на полу
                if (std::abs(fv.velocity.y) < 1.0f)
                {
                    fv.velocity.y = 0.0f;
                    fv.velocity.x *= 0.85f;
                    fv.velocity.z *= 0.85f;
                }
            }
        }

        // Удаляем мёртвых
        fallingVoxels.erase(
            std::remove_if(fallingVoxels.begin(), fallingVoxels.end(),
                           [](const FallingVoxel& f){ return f.dead; }),
            fallingVoxels.end());

        // Перестраиваем меш каждый кадр пока есть падающие —
        // иначе GPU видит позиции из момента разрушения и воксели зависают
        RebuildMesh();
    }

    // ----------------------------------------------------------------
    // Вспомогательные функции меша
    // ----------------------------------------------------------------
    namespace
    {
        void PushFace(
            std::vector<VoxelVertex>& verts,
            std::vector<uint32_t>&    inds,
            const glm::vec3 corners[4],
            const glm::vec3& normal,
            const glm::vec3& color)
        {
            uint32_t base = static_cast<uint32_t>(verts.size());
            for (int i = 0; i < 4; i++)
            {
                VoxelVertex v{};
                v.pos[0] = corners[i].x; v.pos[1] = corners[i].y; v.pos[2] = corners[i].z;
                v.color[0] = color.r;    v.color[1] = color.g;    v.color[2] = color.b;
                v.normal[0] = normal.x;  v.normal[1] = normal.y;  v.normal[2] = normal.z;
                verts.push_back(v);
            }
            inds.push_back(base+0); inds.push_back(base+1); inds.push_back(base+2);
            inds.push_back(base+0); inds.push_back(base+2); inds.push_back(base+3);
        }

        bool IsAlive(const VoxelWall& w, int c, int r)
        {
            if (c < 0 || c >= VOXEL_COLS) return false;
            if (r < 0 || r >= VOXEL_ROWS) return false;
            return w.alive[c][r];
        }

        // Куб из 6 граней, ориентирован по мировым осям (для падающих вокселей)
        void PushCubeWorld(
            std::vector<VoxelVertex>& verts,
            std::vector<uint32_t>&    inds,
            glm::vec3 center, float half,
            glm::vec3 color)
        {
            // +Z
            { glm::vec3 c[4] = {
                {center.x-half, center.y-half, center.z+half},
                {center.x+half, center.y-half, center.z+half},
                {center.x+half, center.y+half, center.z+half},
                {center.x-half, center.y+half, center.z+half}};
              PushFace(verts, inds, c, {0,0,1}, color); }
            // -Z
            { glm::vec3 c[4] = {
                {center.x+half, center.y-half, center.z-half},
                {center.x-half, center.y-half, center.z-half},
                {center.x-half, center.y+half, center.z-half},
                {center.x+half, center.y+half, center.z-half}};
              PushFace(verts, inds, c, {0,0,-1}, color * 0.8f); }
            // +X
            { glm::vec3 c[4] = {
                {center.x+half, center.y-half, center.z+half},
                {center.x+half, center.y-half, center.z-half},
                {center.x+half, center.y+half, center.z-half},
                {center.x+half, center.y+half, center.z+half}};
              PushFace(verts, inds, c, {1,0,0}, color * 0.85f); }
            // -X
            { glm::vec3 c[4] = {
                {center.x-half, center.y-half, center.z-half},
                {center.x-half, center.y-half, center.z+half},
                {center.x-half, center.y+half, center.z+half},
                {center.x-half, center.y+half, center.z-half}};
              PushFace(verts, inds, c, {-1,0,0}, color * 0.85f); }
            // +Y
            { glm::vec3 c[4] = {
                {center.x-half, center.y+half, center.z-half},
                {center.x-half, center.y+half, center.z+half},
                {center.x+half, center.y+half, center.z+half},
                {center.x+half, center.y+half, center.z-half}};
              PushFace(verts, inds, c, {0,1,0}, color * 0.9f); }
            // -Y
            { glm::vec3 c[4] = {
                {center.x-half, center.y-half, center.z+half},
                {center.x-half, center.y-half, center.z-half},
                {center.x+half, center.y-half, center.z-half},
                {center.x+half, center.y-half, center.z+half}};
              PushFace(verts, inds, c, {0,-1,0}, color * 0.7f); }
        }
    }

    // ----------------------------------------------------------------
    // Построение меша стены (face-culled) + падающие вокселя
    // ----------------------------------------------------------------
    void VoxelWall::RebuildMesh()
    {
        vertices.clear();
        indices.clear();

        float rad = glm::radians(rotY);
        glm::mat3 rot = glm::mat3(glm::rotate(glm::mat4(1.0f), rad, glm::vec3(0,1,0)));

        auto lToW = [&](glm::vec3 local) -> glm::vec3
        {
            glm::vec3 r = rot * local;
            return r + glm::vec3(origin.x, origin.y, origin.z);
        };

        const float H = VOXEL_SIZE * 0.5f;

        for (int c = 0; c < VOXEL_COLS; c++)
        {
            for (int r = 0; r < VOXEL_ROWS; r++)
            {
                if (!alive[c][r]) continue;

                float lx = (c - VOXEL_COLS * 0.5f + 0.5f) * VOXEL_SIZE;
                float ly = (r + 0.5f) * VOXEL_SIZE;
                float lz = 0.0f;

                Vec3 vc = GetVoxelColor(c, r);
                glm::vec3 col(vc.x, vc.y, vc.z);

                // +Z face (front) — только если нет живого соседа СПЕРЕДИ
                // (lz фиксирован = 0, сосед по Z не существует в 2D стене,
                // поэтому всегда показываем внешнюю грань)
                {
                    glm::vec3 corners[4] = {
                        lToW({lx-H, ly-H, lz+H}),
                        lToW({lx+H, ly-H, lz+H}),
                        lToW({lx+H, ly+H, lz+H}),
                        lToW({lx-H, ly+H, lz+H}),
                    };
                    PushFace(vertices, indices, corners, rot*glm::vec3(0,0,1), col);
                }

                // -Z face (back)
                {
                    glm::vec3 corners[4] = {
                        lToW({lx+H, ly-H, lz-H}),
                        lToW({lx-H, ly-H, lz-H}),
                        lToW({lx-H, ly+H, lz-H}),
                        lToW({lx+H, ly+H, lz-H}),
                    };
                    PushFace(vertices, indices, corners, rot*glm::vec3(0,0,-1), col*0.8f);
                }

                // +X — только если сосед отсутствует
                if (!IsAlive(*this, c+1, r))
                {
                    glm::vec3 corners[4] = {
                        lToW({lx+H, ly-H, lz+H}),
                        lToW({lx+H, ly-H, lz-H}),
                        lToW({lx+H, ly+H, lz-H}),
                        lToW({lx+H, ly+H, lz+H}),
                    };
                    PushFace(vertices, indices, corners, rot*glm::vec3(1,0,0), col*0.85f);
                }

                // -X
                if (!IsAlive(*this, c-1, r))
                {
                    glm::vec3 corners[4] = {
                        lToW({lx-H, ly-H, lz-H}),
                        lToW({lx-H, ly-H, lz+H}),
                        lToW({lx-H, ly+H, lz+H}),
                        lToW({lx-H, ly+H, lz-H}),
                    };
                    PushFace(vertices, indices, corners, rot*glm::vec3(-1,0,0), col*0.85f);
                }

                // +Y
                if (!IsAlive(*this, c, r+1))
                {
                    glm::vec3 corners[4] = {
                        lToW({lx-H, ly+H, lz-H}),
                        lToW({lx-H, ly+H, lz+H}),
                        lToW({lx+H, ly+H, lz+H}),
                        lToW({lx+H, ly+H, lz-H}),
                    };
                    PushFace(vertices, indices, corners, rot*glm::vec3(0,1,0), col*0.9f);
                }

                // -Y
                if (!IsAlive(*this, c, r-1))
                {
                    glm::vec3 corners[4] = {
                        lToW({lx-H, ly-H, lz+H}),
                        lToW({lx-H, ly-H, lz-H}),
                        lToW({lx+H, ly-H, lz-H}),
                        lToW({lx+H, ly-H, lz+H}),
                    };
                    PushFace(vertices, indices, corners, rot*glm::vec3(0,-1,0), col*0.7f);
                }
            }
        }

        // Добавляем падающие вокселя в тот же меш
        for (const auto& fv : fallingVoxels)
        {
            if (fv.dead) continue;
            glm::vec3 c(fv.color.x, fv.color.y, fv.color.z);
            // Слегка темнеем к концу жизни
            float fade = std::max(0.0f, 1.0f - fv.lifetime / 3.5f);
            PushCubeWorld(vertices, indices,
                          glm::vec3(fv.pos.x, fv.pos.y, fv.pos.z),
                          fv.size * 0.5f,
                          c * fade);
        }

        meshDirty = true;
    }

    // ----------------------------------------------------------------
    // Создание 4 стен комнаты
    // ----------------------------------------------------------------
    std::vector<VoxelWall> CreateRoomWalls()
    {
        std::vector<VoxelWall> walls;
        walls.resize(4);

        float wallWidth = VOXEL_COLS * VOXEL_SIZE;
        float roomHalf  = wallWidth * 1.25f;

        walls[0].Init(Vec3(0.0f, 0.0f, -roomHalf), 0.0f,   0);
        walls[1].Init(Vec3(0.0f, 0.0f,  roomHalf), 180.0f, 1);
        walls[2].Init(Vec3(-roomHalf, 0.0f, 0.0f), 90.0f,  2);
        walls[3].Init(Vec3( roomHalf, 0.0f, 0.0f), 270.0f, 3);

        return walls;
    }
}
