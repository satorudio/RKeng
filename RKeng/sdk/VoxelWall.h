#pragma once
#include "MathTypes.h"
#include <vector>
#include <cstdint>
#include <array>

namespace RKeng
{
    // Падающий воксель — отделился от стены и летит вниз
    struct FallingVoxel
    {
        Vec3  pos;          // мировая позиция центра
        Vec3  velocity;     // текущая скорость (м/с)
        Vec3  color;        // цвет (сохраняем при отделении)
        float size;         // размер стороны (= VOXEL_SIZE)
        float lifetime = 0.0f;   // прошло секунд с момента отделения
        bool  dead     = false;  // пометить к удалению
    };
}

// VoxelWall.h — воксельная разрушаемая стена.
// Стена — это сетка VOXEL_COLS x VOXEL_ROWS вокселей размером VOXEL_SIZE.
// При выстреле воксель удаляется из физики и меша.

#ifdef RK_JOLT_ENABLED
// Jolt требует Jolt.h первым — он определяет JPH_NAMESPACE_BEGIN/END и все базовые типы
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#endif

namespace RKeng
{
    constexpr float VOXEL_SIZE  = 0.25f;
    constexpr int   VOXEL_COLS  = 16;
    constexpr int   VOXEL_ROWS  = 10;

    struct VoxelVertex
    {
        float pos[3];
        float normal[3];
        float color[3];
    };

    struct VoxelWall
    {
        Vec3  origin { 0, 0, 0 };
        float rotY   = 0.0f;

        std::array<std::array<bool, VOXEL_ROWS>, VOXEL_COLS> alive {};

        std::vector<VoxelVertex> vertices;
        std::vector<uint32_t>    indices;

        bool     meshDirty = true;
        uint32_t id        = 0;

        // Отлетевшие вокселя — симулируются отдельно
        std::vector<FallingVoxel> fallingVoxels;

#ifdef RK_JOLT_ENABLED
        std::array<JPH::BodyID, VOXEL_COLS * VOXEL_ROWS> voxelBodyIDs {};
#endif

        void  Init(Vec3 pos, float rotationY, uint32_t wallID);

        // Разрушить воксель: убирает из alive, создаёт FallingVoxel с импульсом
        bool  DestroyVoxel(int col, int row, Vec3 impulse = {0,0,0});
        bool  DestroyRadius(Vec3 worldHitPos, float radius);
        void  RebuildMesh();
        Vec3  VoxelWorldPos(int col, int row) const;

        // Обновить симуляцию падающих вокселей (вызывается каждый тик)
        void  UpdateFalling(float dt);

        // Построить меш из падающих вокселей (добавляется к основному)
        void  RebuildFallingMesh();

        Vec3  GetVoxelColor(int col, int row) const;
    };

    std::vector<VoxelWall> CreateRoomWalls();
}
