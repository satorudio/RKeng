#pragma once
#include "Camera.h"
#include <array>
namespace RKeng {
    struct Frustum { std::array<Vec4,6> planes; };
}
namespace RKeng::FrustumOps {
    Frustum BuildFromCamera(const Camera& c);
    bool ContainsPoint(const Frustum& f, const Vec3& p);
    bool ContainsAABB(const Frustum& f, const Vec3& min, const Vec3& max);
}
