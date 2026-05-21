#pragma once
#include "../core/SceneState.h"

// Одна задача: читать ввод и писать в InputState.
// Сейчас stub под GLFW — заменяется без касания остального кода.

namespace RKeng::InputPoll
{
    // running → false если окно закрыли
    void Run(InputState& input, bool& running);
}
