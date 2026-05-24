// CarInputPoll.cpp — GLFW-опрос для пикапа.
// W/S — газ/тормоз,  A/D — руль,  Space — ручник.
// ПКМ + мышь — вращение камеры,  Q/E — зум.

#include "CarInputPoll.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <cmath>

namespace RKeng::CarInputPoll
{
    static float  s_camYaw   = 0.0f;
    static float  s_camPitch = -15.0f;
    static float  s_camDist  = 10.0f;
    static double s_lastX    = 0.0;
    static double s_lastY    = 0.0;
    static bool   s_init     = false;
    static bool   s_rmb      = false;

    void Run(CarState& car, const SceneState& scene, float dt)
    {
        GLFWwindow* win = scene.windowHandle;
        if (!win) win = glfwGetCurrentContext();
        if (!win) return;

        auto& inp = car.input;
        const float rampUp   = dt * 3.0f;
        const float rampDown = dt * 5.0f;

        // Газ
        bool wDown = (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS);
        inp.throttle = wDown
            ? glm::min(1.0f, inp.throttle + rampUp)
            : glm::max(0.0f, inp.throttle - rampDown);

        // Тормоз
        bool sDown = (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS);
        inp.brake = sDown
            ? glm::min(1.0f, inp.brake + rampUp)
            : glm::max(0.0f, inp.brake - rampDown);

        // Руль
        bool aDown = (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS);
        bool dDown = (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS);
        const float sr = dt * 2.5f, sret = dt * 6.0f;
        if (aDown && !dDown)
            inp.steer = glm::max(-1.0f, inp.steer - sr);
        else if (dDown && !aDown)
            inp.steer = glm::min( 1.0f, inp.steer + sr);
        else {
            float step = glm::min(std::abs(inp.steer), sret);
            inp.steer += inp.steer > 0.0f ? -step : step;
        }

        // Ручник
        inp.handbrake = (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS);

        // Камера — ПКМ + мышь
        bool rmbNow = (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
        double mx, my;
        glfwGetCursorPos(win, &mx, &my);

        if (!s_init) { s_lastX = mx; s_lastY = my; s_init = true; }

        if (rmbNow && s_rmb) {
            s_camYaw   += (float)(mx - s_lastX) * 0.20f;
            s_camPitch -= (float)(my - s_lastY) * 0.20f;
            s_camPitch  = glm::clamp(s_camPitch, -70.0f, 30.0f);
        }
        s_lastX = mx; s_lastY = my; s_rmb = rmbNow;

        // Зум Q/E
        if (glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS) s_camDist = glm::max(3.0f,  s_camDist - 8.0f*dt);
        if (glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS) s_camDist = glm::min(30.0f, s_camDist + 8.0f*dt);

        car.camYaw   = s_camYaw;
        car.camPitch = s_camPitch;
        car.camDist  = s_camDist;
    }
}
