#include "core/Engine.h"
#include "utils/Logger.h"
#include <exception>
#include <iostream>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>

LONG WINAPI VehCrashHandler(EXCEPTION_POINTERS* ep)
{
    DWORD code = ep->ExceptionRecord->ExceptionCode;
    if (code == 0xE06D7363) return EXCEPTION_CONTINUE_SEARCH;

    std::cerr << "[VEH] Exception: 0x" << std::hex << code
              << " at 0x" << (uintptr_t)ep->ExceptionRecord->ExceptionAddress
              << std::dec << "\n";

    // База DLL
    HMODULE hDll = GetModuleHandleA("libOpenCarWorld.dll");
    std::cerr << "[VEH] libOpenCarWorld.dll base: 0x" << std::hex << (uintptr_t)hDll << "\n";

    void* stack[32];
    USHORT frames = CaptureStackBackTrace(0, 32, stack, nullptr);
    std::cerr << "[VEH] Stack (" << frames << " frames):\n";
    for (USHORT i = 0; i < frames; i++)
        std::cerr << "  #" << i << " 0x" << std::hex << (uintptr_t)stack[i] << "\n";
    std::cerr << std::dec << std::flush;

    return EXCEPTION_CONTINUE_SEARCH;
}

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
    AddVectoredExceptionHandler(1, VehCrashHandler);
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
