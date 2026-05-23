#pragma once
#include "../core/SceneState.h"

// Одна задача: читать ввод и писать в InputState.
// Сейчас stub под GLFW — заменяется без касания остального кода.

namespace RKeng::InputPoll
{
    void Init();  // вызвать один раз после создания окна — регистрирует focus callback
    void Run(InputState& input, bool& running);
}
