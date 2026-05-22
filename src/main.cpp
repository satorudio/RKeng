#include "core/Engine.h"
#include "utils/Logger.h"
#include <exception>
#include <iostream>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>

// WIN32 subsystem (нужен для Adrenalin/overlays): нет консоли по умолчанию.
// AllocConsole открывает окно консоли вручную — логи остаются видны при разработке.
// В Release можно убрать AllocConsole и логи пойдут только в файл.
static void AttachConsoleOutput()
{
    if (!AllocConsole()) return;  // уже есть — ок
    // Переподключаем stdout/stderr к новой консоли
    FILE* fp = nullptr;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONIN$",  "r", stdin);
    std::cout.clear(); std::cerr.clear(); std::cin.clear();
}

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

static int RKengMain()
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

#ifdef _WIN32
// WIN32 subsystem: точка входа WinMain.
// Adrenalin 26.x определяет "игру" по SUBSYSTEM:WINDOWS в PE-заголовке.
// Консольный subsystem (int main) он игнорирует.
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    AttachConsoleOutput();  // открываем консоль для логов при разработке
    return RKengMain();
}
#else
int main()
{
    return RKengMain();
}
#endif
