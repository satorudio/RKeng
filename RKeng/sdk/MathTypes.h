#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace RKeng
{
    using Vec2 = glm::vec2; using Vec3 = glm::vec3;
    using Vec4 = glm::vec4; using Mat3 = glm::mat3;
    using Mat4 = glm::mat4; using Quat = glm::quat;
    using DVec3 = glm::dvec3; using DVec2 = glm::dvec2;

    struct WorldPos {
        DVec3 world{ 0.0, 0.0, 0.0 };
        Vec3 ToLocal(const DVec3& origin) const { return Vec3(world - origin); }
    };

    constexpr float PI = glm::pi<float>();
    constexpr float DEG2RAD = glm::pi<float>() / 180.0f;
    constexpr float RAD2DEG = 180.0f / glm::pi<float>();
    constexpr double ORIGIN_SHIFT_THRESHOLD = 500.0;
}
