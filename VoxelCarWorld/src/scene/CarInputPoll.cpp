// CarInputPoll.cpp — опрос управления RAM 2500 Power Wagon через SceneState.
//
// Движок заполняет scene.input (forward/backward/left/right, mouseDeltaX/Y, yaw, pitch)
// и scene.windowHandle перед каждым OnTick. DLL не вызывает GLFW напрямую.
//
// Управление:
//   W / forward  — газ
//   S / backward — тормоз / задний ход
//   A / left     — руль влево
//   D / right    — руль вправо
//   jump (Space) — ручной тормоз
//   run  (Shift) — пониженная передача 4L toggle
//   Движение мыши (ПКМ зажата в движке) — поворот камеры через mouseDeltaX/Y
//   Q / E        — зум камеры (через windowHandle если доступен, иначе пропускаем)

#include "CarInputPoll.h"
#include <glm/glm.hpp>
#include <cmath>

// Ввод полностью через SceneState — GLFW не используется.

namespace RKeng::CarInputPoll
{
    static float  s_camYaw    = 0.0f;
    static float  s_camPitch  = -12.0f;
    static float  s_camDist   = 10.0f;

    // Пониженная передача — toggle при нажатии run (Shift)
    static bool   s_lowRangePrev = false;
    static bool   s_lowRangeOn  = false;

    void Run(CarState& car, const SceneState& scene, float dt)
    {
        auto& inp = car.input;
        const auto& si = scene.input;

        // ── Газ ──────────────────────────────────────────────────────────
        const float rampUp   = dt * 2.8f;
        const float rampDown = dt * 5.0f;

        inp.throttle = si.forward
            ? glm::min(1.0f, inp.throttle + rampUp)
            : glm::max(0.0f, inp.throttle - rampDown);

        // ── Тормоз ───────────────────────────────────────────────────────
        inp.brake = si.backward
            ? glm::min(1.0f, inp.brake + rampUp)
            : glm::max(0.0f, inp.brake - rampDown);

        // ── Руль ─────────────────────────────────────────────────────────
        const float steerRate   = dt * 2.8f;
        const float steerReturn = dt * 6.0f;

        if (si.left && !si.right)
            inp.steer = glm::max(-1.0f, inp.steer - steerRate);
        else if (si.right && !si.left)
            inp.steer = glm::min( 1.0f, inp.steer + steerRate);
        else
        {
            float step = glm::min(std::abs(inp.steer), steerReturn);
            inp.steer += (inp.steer > 0.0f ? -step : step);
        }

        // ── Ручник (Space / jump) ─────────────────────────────────────────
        inp.handbrake = si.jump;

        // ── Пониженная 4L — toggle по Shift (run) ─────────────────────────
        if (si.run && !s_lowRangePrev)
            s_lowRangeOn = !s_lowRangeOn;
        s_lowRangePrev = si.run;
        inp.lowRange   = s_lowRangeOn;

        // ── Камера — берём yaw/pitch напрямую из InputState ───────────────
        // Движок накапливает их из mouseDelta при зажатой ПКМ.
        s_camYaw   = si.yaw;
        s_camPitch = glm::clamp(si.pitch, -75.0f, 35.0f);

        // ── Зум — TODO: добавить в InputState движка (Q/E или scroll) ──────

        // Записываем в CarState
        car.camYaw         = s_camYaw;
        car.camPitch       = s_camPitch;
        car.camLocalOffset = Vec3(0.0f, 1.8f, -s_camDist);
    }

}  // namespace RKeng::CarInputPoll
