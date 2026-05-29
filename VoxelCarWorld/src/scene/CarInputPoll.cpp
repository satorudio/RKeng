// CarInputPoll.cpp — GLFW-опрос для пикапа, первое лицо.
// W/S — газ/тормоз,  A/D — руль,  Space — ручник.
// Мышь — вращение взгляда (всегда, курсор захвачен движком).

#include "CarInputPoll.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <cmath>

namespace RKeng::CarInputPoll
{
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

        // Взгляд от первого лица — мышь всегда вращает (курсор захвачен движком)
        // Переключение fly cam по M
        bool mNow = (glfwGetKey(win, GLFW_KEY_M) == GLFW_PRESS);
        if (mNow && !car.mKeyWas) {
            car.flyCamActive = !car.flyCamActive;
            if (car.flyCamActive) {
                // Инициализируем позицию fly cam из текущей позиции камеры
                car.flyCamX     = car.position.x;
                car.flyCamY     = car.position.y + car.params.halfH * 0.3f;
                car.flyCamZ     = car.position.z;
                car.flyCamYaw   = car.camYaw;
                car.flyCamPitch = car.camPitch;
            }
        }
        car.mKeyWas = mNow;

        if (!car.flyCamActive) {
            // Обычная камера машины
            car.camYaw   += scene.input.mouseDeltaX * 0.15f;
            car.camPitch -= scene.input.mouseDeltaY * 0.15f;
            car.camPitch  = glm::clamp(car.camPitch, -80.0f, 80.0f);
        } else {
            // Fly cam — мышь вращает
            car.flyCamYaw   += scene.input.mouseDeltaX * 0.15f;
            car.flyCamPitch -= scene.input.mouseDeltaY * 0.15f;
            car.flyCamPitch  = glm::clamp(car.flyCamPitch, -89.0f, 89.0f);

            // Движение WASD в fly cam (независимо от машины)
            float yawR = glm::radians(car.flyCamYaw);
            float spd  = car.flyCamSpeed * dt;
            float fx   =  glm::sin(yawR);
            float fz   = -glm::cos(yawR);

            bool fwd  = (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS);
            bool back = (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS);
            bool left = (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS);
            bool right= (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS);
            bool up   = (glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS);
            bool down = (glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS);

            if (fwd)  { car.flyCamX += fx*spd; car.flyCamZ += fz*spd; }
            if (back) { car.flyCamX -= fx*spd; car.flyCamZ -= fz*spd; }
            if (left) { car.flyCamX -= fz*spd; car.flyCamZ += fx*spd; }
            if (right){ car.flyCamX += fz*spd; car.flyCamZ -= fx*spd; }
            if (up)   { car.flyCamY += spd; }
            if (down) { car.flyCamY -= spd; }
        }
    }
}
