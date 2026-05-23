#include "Engine.h"
#include "EngineInit.h"
#include "EngineLoop.h"
#include "EngineShutdown.h"

namespace RKeng
{
    void Engine::Init()
    {
        EngineInit::Run(m_Running);
    }

    void Engine::Run()
    {
        EngineLoop::Run(m_Running);
    }

    void Engine::Shutdown()
    {
        EngineShutdown::Run();
    }
}
