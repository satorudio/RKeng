#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
namespace RKeng::CameraOps {
    Mat4 BuildView(const Camera& c) { return glm::lookAt(c.position, c.target, c.up); }
    Mat4 BuildProjection(const Camera& c) {
        Mat4 p = glm::perspective(glm::radians(c.fovDeg), c.aspect, c.nearPlane, c.farPlane);
        p[1][1] *= -1.0f; return p;
    }
}
