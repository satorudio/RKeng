#include "CarInputPoll.h"
#include "../window/WindowState.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace RKeng::CarInputPoll
{
    static float s_yaw   = 0.0f;
    static float s_pitch = 0.0f;

    void Run(CarState& car, const SceneState& /*scene*/, float dt)
    {
        GLFWwindow* win = GetWindowState().handle;
        if (!win) return;
        auto& inp = car.input;

        // ---- Газ / тормоз ----------------------------------------------
        bool wPressed = glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS;
        bool sPressed = glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS;

        const float rampUp   = dt * 2.5f;
        const float rampDown = dt * 4.0f;

        if (wPressed)
            inp.throttle = glm::min(1.0f, inp.throttle + rampUp);
        else
            inp.throttle = glm::max(0.0f, inp.throttle - rampDown);

        if (sPressed)
            inp.brake = glm::min(1.0f, inp.brake + rampUp);
        else
            inp.brake = glm::max(0.0f, inp.brake - rampDown);

        // ---- Руль ------------------------------------------------------
        bool aPressed = glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS;
        bool dPressed = glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS;

        const float steerRate   = dt * 3.0f;
        const float steerReturn = dt * 5.0f;

        if (aPressed && !dPressed)
            inp.steer = glm::max(-1.0f, inp.steer - steerRate);
        else if (dPressed && !aPressed)
            inp.steer = glm::min( 1.0f, inp.steer + steerRate);
        else
        {
            if (inp.steer > 0) inp.steer = glm::max(0.0f, inp.steer - steerReturn);
            else               inp.steer = glm::min(0.0f, inp.steer + steerReturn);
        }

        // ---- Ручник ----------------------------------------------------
        inp.handbrake = (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT)  == GLFW_PRESS ||
                         glfwGetKey(win, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);

        // ---- Камера (мышь) ---------------------------------------------
        static double s_lastX = 0, s_lastY = 0;
        static bool   s_first = true;
        double mx, my;
        glfwGetCursorPos(win, &mx, &my);

        if (s_first) { s_lastX = mx; s_lastY = my; s_first = false; }

        double dx = mx - s_lastX;
        double dy = my - s_lastY;
        s_lastX = mx; s_lastY = my;

        const float sens = 0.15f;
        s_yaw   += (float)dx * sens;
        s_pitch -= (float)dy * sens;
        s_pitch  = glm::clamp(s_pitch, -80.0f, 30.0f);

        car.camYaw   = s_yaw;
        car.camPitch = s_pitch;
    }
}
