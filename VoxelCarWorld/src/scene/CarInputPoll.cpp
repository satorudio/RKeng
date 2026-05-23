// CarInputPoll.cpp — GLFW-опрос для RAM 2500 Power Wagon.
//
// Управление:
//   W           — газ
//   S           — тормоз / задний ход (при скорости < 0.5)
//   A / D       — руль
//   Space       — ручной тормоз
//   L           — пониженная передача (4L / 4H toggle)
//   Правая кнопка мыши + движение — поворот камеры
//   Scroll      — зум камеры (расстояние)

#include "CarInputPoll.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <cmath>

namespace RKeng::CarInputPoll
{
    // Стейт камеры — статик, один на DLL
    static float  s_camYaw    = 0.0f;
    static float  s_camPitch  = -12.0f;  // слегка смотрим сверху вниз по умолч.
    static float  s_camDist   = 10.0f;   // расстояние от машины
    static double s_lastX     = 0.0;
    static double s_lastY     = 0.0;
    static bool   s_firstMove = true;
    static bool   s_rmb       = false;   // правая кнопка зажата

    // Пониженная передача — toggle при нажатии L (edge detect)
    static bool   s_lowRangePrev = false;
    static bool   s_lowRangeOn  = false;

    void Run(CarState& car, const SceneState& /*scene*/, float dt)
    {
        GLFWwindow* win = glfwGetCurrentContext();
        if (!win) return;

        auto& inp = car.input;

        // ── Газ ──────────────────────────────────────────────────────────
        const bool wDown = (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS);
        const float rampUp   = dt * 2.8f;
        const float rampDown = dt * 5.0f;

        inp.throttle = wDown
            ? glm::min(1.0f, inp.throttle + rampUp)
            : glm::max(0.0f, inp.throttle - rampDown);

        // ── Тормоз ───────────────────────────────────────────────────────
        const bool sDown = (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS);
        inp.brake = sDown
            ? glm::min(1.0f, inp.brake + rampUp)
            : glm::max(0.0f, inp.brake - rampDown);

        // ── Руль ─────────────────────────────────────────────────────────
        const bool aDown = (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS);
        const bool dDown = (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS);

        const float steerRate   = dt * 2.8f;
        const float steerReturn = dt * 6.0f;

        if (aDown && !dDown)
            inp.steer = glm::max(-1.0f, inp.steer - steerRate);
        else if (dDown && !aDown)
            inp.steer = glm::min( 1.0f, inp.steer + steerRate);
        else
        {
            float step = glm::min(std::abs(inp.steer), steerReturn);
            inp.steer += (inp.steer > 0.0f ? -step : step);
        }

        // ── Ручник ────────────────────────────────────────────────────────
        inp.handbrake =
            (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS);

        // ── Пониженная (4L) — toggle ──────────────────────────────────────
        const bool lDown = (glfwGetKey(win, GLFW_KEY_L) == GLFW_PRESS);
        if (lDown && !s_lowRangePrev)
            s_lowRangeOn = !s_lowRangeOn;
        s_lowRangePrev = lDown;
        inp.lowRange   = s_lowRangeOn;

        // ── Камера ────────────────────────────────────────────────────────
        // Правая кнопка мыши — держать для вращения
        const bool rmbNow = (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);

        double mx, my;
        glfwGetCursorPos(win, &mx, &my);

        if (s_firstMove || !s_rmb)
        {
            s_lastX = mx; s_lastY = my;
            if (!s_firstMove && rmbNow) s_firstMove = false;
            else s_firstMove = false;
        }

        if (rmbNow)
        {
            const float sens = 0.18f;
            s_camYaw   += (float)(mx - s_lastX) * sens;
            s_camPitch -= (float)(my - s_lastY) * sens;
            s_camPitch  = glm::clamp(s_camPitch, -75.0f, 35.0f);
        }

        s_lastX = mx;
        s_lastY = my;
        s_rmb   = rmbNow;

        // Скролл — зум (через window user pointer не доступен напрямую,
        // эмулируем Q/E для изменения дистанции камеры)
        const bool qDown = (glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS);
        const bool eDown = (glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS);
        if (qDown) s_camDist = glm::max(3.0f,  s_camDist - 8.0f * dt);
        if (eDown) s_camDist = glm::min(35.0f, s_camDist + 8.0f * dt);

        // Записываем в CarState
        car.camYaw          = s_camYaw;
        car.camPitch        = s_camPitch;
        car.camLocalOffset  = Vec3(0.0f, 1.8f, -s_camDist);
    }

}  // namespace RKeng::CarInputPoll
