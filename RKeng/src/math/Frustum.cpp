#include "Frustum.h"
#include "CameraOps.h"
#include <cmath>

namespace RKeng::FrustumOps
{
    // Извлекаем 6 плоскостей фрустума из VP-матрицы (метод Gribb-Hartmann).
    // Плоскости в world space, нормаль смотрит ВНУТРЬ фрустума.
    Frustum BuildFromCamera(const Camera& c)
    {
        Mat4 view = CameraOps::BuildView(c);
        Mat4 rawProj = glm::perspective(
            glm::radians(c.fovDeg), c.aspect, c.nearPlane, c.farPlane);
        Mat4 vp = rawProj * view;

        Frustum f;
        // Left:   col3 + col0
        f.planes[0] = Vec4(vp[0][3] + vp[0][0],
                           vp[1][3] + vp[1][0],
                           vp[2][3] + vp[2][0],
                           vp[3][3] + vp[3][0]);
        // Right:  col3 - col0
        f.planes[1] = Vec4(vp[0][3] - vp[0][0],
                           vp[1][3] - vp[1][0],
                           vp[2][3] - vp[2][0],
                           vp[3][3] - vp[3][0]);
        // Bottom: col3 + col1
        f.planes[2] = Vec4(vp[0][3] + vp[0][1],
                           vp[1][3] + vp[1][1],
                           vp[2][3] + vp[2][1],
                           vp[3][3] + vp[3][1]);
        // Top:    col3 - col1
        f.planes[3] = Vec4(vp[0][3] - vp[0][1],
                           vp[1][3] - vp[1][1],
                           vp[2][3] - vp[2][1],
                           vp[3][3] - vp[3][1]);
        // Near:   col3 + col2
        f.planes[4] = Vec4(vp[0][3] + vp[0][2],
                           vp[1][3] + vp[1][2],
                           vp[2][3] + vp[2][2],
                           vp[3][3] + vp[3][2]);
        // Far:    col3 - col2
        f.planes[5] = Vec4(vp[0][3] - vp[0][2],
                           vp[1][3] - vp[1][2],
                           vp[2][3] - vp[2][2],
                           vp[3][3] - vp[3][2]);

        // Нормализуем (нужно для ContainsPoint; ContainsAABB — нет, но чище)
        for (auto& p : f.planes) {
            float len = std::sqrt(p.x*p.x + p.y*p.y + p.z*p.z);
            if (len > 1e-6f) p /= len;
        }
        return f;
    }

    bool ContainsPoint(const Frustum& f, const Vec3& p)
    {
        for (const auto& plane : f.planes)
            if (plane.x*p.x + plane.y*p.y + plane.z*p.z + plane.w < 0.f)
                return false;
        return true;
    }

    // AABB vs фрустум — positive-vertex тест (быстрый, нет false-negative)
    bool ContainsAABB(const Frustum& f, const Vec3& mn, const Vec3& mx)
    {
        for (const auto& p : f.planes) {
            // Выбираем "позитивный" вершину AABB (дальше всего по направлению нормали)
            Vec3 pv {
                p.x >= 0.f ? mx.x : mn.x,
                p.y >= 0.f ? mx.y : mn.y,
                p.z >= 0.f ? mx.z : mn.z
            };
            if (p.x*pv.x + p.y*pv.y + p.z*pv.z + p.w < 0.f)
                return false; // полностью снаружи этой плоскости
        }
        return true;
    }
}
