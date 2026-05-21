#include "core/Engine.h"
#include "utils/Logger.h"
#include <exception>
#include <iostream>
#include <cstdlib>

// Ловим даже structured exceptions (access violation) на Windows
#ifdef _WIN32
#include <windows.h>
LONG WINAPI CrashHandler(EXCEPTION_POINTERS* ep)
{
    std::cerr << "[FATAL] Structured exception code: 0x"
              << std::hex << ep->ExceptionRecord->ExceptionCode << '\n' << std::flush;
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

int main()
{
#ifdef _WIN32
    SetUnhandledExceptionFilter(CrashHandler);
#endif

    try
    {
        RKeng::Engine engine;
        engine.Init();
        engine.Run();
        engine.Shutdown();
    }
    catch (const std::exception& e)
    {
        std::cerr << "[FATAL] " << e.what() << '\n' << std::flush;
        return 1;
    }
    catch (...)
    {
        std::cerr << "[FATAL] Unknown exception\n" << std::flush;
        return 1;
    }
    return 0;
}
