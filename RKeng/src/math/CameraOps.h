#pragma once
#include "MathTypes.h"

namespace RKeng {
    struct Camera {
        Vec3  position  { 0,1,0 };
        Vec3  target    { 0,1,-1 };
        Vec3  up        { 0,1,0 };
        float fovDeg    = 90.0f;
        float aspect    = 16.0f/9.0f;
        float nearPlane = 0.05f;
        float farPlane  = 500.0f;
    };
}
namespace RKeng::CameraOps {
    Mat4 BuildView(const Camera& c);
    Mat4 BuildProjection(const Camera& c);
}
