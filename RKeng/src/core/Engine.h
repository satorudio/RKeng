#pragma once

// Engine.h — дирижёр верхнего уровня.
// Сам ничего не делает: только владеет подсистемами и вызывает их.

#include "../../engine_api/IScenePlugin.h"  // для RK_API

namespace RKeng
{
    class RK_API Engine
    {
    public:
        Engine()  = default;
        ~Engine() = default;

        void Init();
        void Run();
        void Shutdown();

    private:
        bool m_Running = false;
    };
}
