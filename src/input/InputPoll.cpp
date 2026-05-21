#include "InputPoll.h"
#include "../window/WindowState.h"
#include <GLFW/glfw3.h>

namespace RKeng::InputPoll
{
    static double s_LastMouseX = 0.0;
    static double s_LastMouseY = 0.0;
    static bool   s_FirstMouse = true;

    void Run(InputState& input, bool& running)
    {
        // Сброс однокадровых значений
        input.mouseDeltaX = 0.0f;
        input.mouseDeltaY = 0.0f;
        input.jumpPressed = false;

        glfwPollEvents();

        auto* handle = GetWindowState().handle;
        if (!handle || glfwWindowShouldClose(handle))
        {
            running = false;
            return;
        }

        // Клавиши
        input.forward  = glfwGetKey(handle, GLFW_KEY_W)            == GLFW_PRESS;
        input.backward = glfwGetKey(handle, GLFW_KEY_S)            == GLFW_PRESS;
        input.left     = glfwGetKey(handle, GLFW_KEY_A)            == GLFW_PRESS;
        input.right    = glfwGetKey(handle, GLFW_KEY_D)            == GLFW_PRESS;
        input.crouch   = glfwGetKey(handle, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
        input.run      = glfwGetKey(handle, GLFW_KEY_LEFT_SHIFT)   == GLFW_PRESS;

        // Пробел: edge detection — jumpPressed только в первый кадр нажатия
        bool spaceNow    = glfwGetKey(handle, GLFW_KEY_SPACE) == GLFW_PRESS;
        input.jumpPressed = spaceNow && !input.jump;
        input.jump        = spaceNow;

        // Escape — отпустить курсор / выход
        if (glfwGetKey(handle, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            running = false;
            return;
        }

        // Мышь
        double mx, my;
        glfwGetCursorPos(handle, &mx, &my);

        if (s_FirstMouse)
        {
            s_LastMouseX = mx;
            s_LastMouseY = my;
            s_FirstMouse = false;
        }

        input.mouseDeltaX = static_cast<float>(mx - s_LastMouseX);
        input.mouseDeltaY = static_cast<float>(my - s_LastMouseY);
        s_LastMouseX = mx;
        s_LastMouseY = my;
    }
}
