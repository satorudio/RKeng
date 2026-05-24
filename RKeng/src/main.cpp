#include "core/Engine.h"
#include "utils/Logger.h"
#include <exception>
#include <iostream>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <tlhelp32.h>
#include <dbghelp.h>

// ─────────────────────────────────────────────────────────────────────────────
//  Низкоуровневый лог в файл — работает до CRT, до Logger, до всего.
//  Используется из VEH где std::cerr может быть ещё не настроен.
// ─────────────────────────────────────────────────────────────────────────────
static HANDLE g_logFile = INVALID_HANDLE_VALUE;

static void RawLog(const char* msg)
{
    if (g_logFile == INVALID_HANDLE_VALUE) return;
    DWORD written = 0;
    WriteFile(g_logFile, msg, (DWORD)strlen(msg), &written, nullptr);
    FlushFileBuffers(g_logFile);
}

static void RawLogHex(const char* prefix, uintptr_t val)
{
    char buf[64];
    // простой hex без printf
    const char* hex = "0123456789ABCDEF";
    char tmp[20]; int n = 0;
    uintptr_t v = val;
    do { tmp[n++] = hex[v & 0xF]; v >>= 4; } while (v);
    char out[64]; int oi = 0;
    while (prefix[oi]) { out[oi] = prefix[oi]; oi++; }
    out[oi++] = '0'; out[oi++] = 'x';
    for (int i = n-1; i >= 0; i--) out[oi++] = tmp[i];
    out[oi++] = '\n'; out[oi] = 0;
    RawLog(out);
}

// ─────────────────────────────────────────────────────────────────────────────
//  VEH — регистрируется как можно раньше (до RKengCore загрузки)
// ─────────────────────────────────────────────────────────────────────────────
LONG WINAPI VehCrashHandler(EXCEPTION_POINTERS* ep)
{
    DWORD code = ep->ExceptionRecord->ExceptionCode;
    if (code == 0xE06D7363) return EXCEPTION_CONTINUE_SEARCH; // MSVC C++ exception
    if (code == 0x20474343) return EXCEPTION_CONTINUE_SEARCH; // GCC  C++ exception (libgcc_s_seh)

    // Пишем сырым WriteFile — cerr/cout могут быть ещё не готовы
    RawLog("\n[VEH] *** CRASH ***\n");
    switch (code) {
        case 0xC0000005: RawLog("[VEH] ACCESS_VIOLATION\n"); break;
        case 0x80000003: RawLog("[VEH] BREAKPOINT (Jolt JPH_ASSERT failed)\n"); break;
        case 0xC0000094: RawLog("[VEH] INT_DIVIDE_BY_ZERO\n"); break;
        case 0xC00000FD: RawLog("[VEH] STACK_OVERFLOW\n"); break;
        default: {
            char buf[64] = "[VEH] ExceptionCode: 0x";
            const char* hx = "0123456789ABCDEF";
            int i = 23; DWORD c2 = code;
            char tmp[10]; int n=0;
            do { tmp[n++]= hx[c2&0xF]; c2>>=4; } while(c2);
            for(int j=n-1;j>=0;j--) buf[i++]=tmp[j];
            buf[i++]='\n'; buf[i]=0;
            RawLog(buf);
        }
    }
    RawLogHex("[VEH] At: ", (uintptr_t)ep->ExceptionRecord->ExceptionAddress);

    // Список загруженных модулей
    RawLog("[VEH] Modules:\n");
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        MODULEENTRY32 me{}; me.dwSize = sizeof(me);
        uintptr_t crashAddr = (uintptr_t)ep->ExceptionRecord->ExceptionAddress;
        if (Module32First(hSnap, &me)) {
            do {
                uintptr_t base = (uintptr_t)me.modBaseAddr;
                uintptr_t end  = base + me.modBaseSize;
                char line[512];
                wsprintfA(line, "  %-40s base=0x%p size=0x%08X%s\n",
                    me.szModule, (void*)base, me.modBaseSize,
                    (crashAddr >= base && crashAddr < end) ? "  <-- CRASH HERE" : "");
                RawLog(line);
            } while (Module32Next(hSnap, &me));
        }
        CloseHandle(hSnap);
    }

    // Символьный стэктрейс
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
    HANDLE hProc = GetCurrentProcess();
    SymInitialize(hProc, nullptr, TRUE);

    void* stack[48];
    USHORT frames = CaptureStackBackTrace(0, 48, stack, nullptr);
    RawLog("[VEH] Stack:\n");

    char symBuf[sizeof(SYMBOL_INFO) + 256] = {};
    SYMBOL_INFO* sym = reinterpret_cast<SYMBOL_INFO*>(symBuf);
    sym->MaxNameLen   = 255;
    sym->SizeOfStruct = sizeof(SYMBOL_INFO);

    IMAGEHLP_LINE64 lineInfo{}; lineInfo.SizeOfStruct = sizeof(lineInfo);
    DWORD lineDisp = 0;

    for (USHORT i = 0; i < frames; i++) {
        DWORD64 addr = (DWORD64)(uintptr_t)stack[i];
        DWORD64 disp = 0;
        char frameLine[1024];
        if (SymFromAddr(hProc, addr, &disp, sym)) {
            if (SymGetLineFromAddr64(hProc, addr, &lineDisp, &lineInfo))
                wsprintfA(frameLine, "  #%2d  %s+%I64u  %s:%lu\n",
                    i, sym->Name, disp, lineInfo.FileName, lineInfo.LineNumber);
            else
                wsprintfA(frameLine, "  #%2d  %s+%I64u\n", i, sym->Name, disp);
        } else {
            wsprintfA(frameLine, "  #%2d  0x%016I64X  (no symbol)\n", i, addr);
        }
        RawLog(frameLine);
    }
    RawLog("[VEH] --- end of crash report ---\n");
    FlushFileBuffers(g_logFile);

    return EXCEPTION_CONTINUE_SEARCH;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Консоль + файл лога
// ─────────────────────────────────────────────────────────────────────────────
static void AttachConsoleOutput()
{
    // Консольное окно
    if (!AttachConsole(ATTACH_PARENT_PROCESS))
        AllocConsole();

    FILE* fp = nullptr;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONIN$",  "r", stdin);
    std::cout.clear(); std::cerr.clear(); std::cin.clear();

    // Дублируем в файл через tee-подход: оба handle открыты одновременно
    // (g_logFile уже открыт до этого — не закрываем)
    std::cout << "[main] console attached, log: rkeng.log\n" << std::flush;
}

LONG WINAPI CrashHandler(EXCEPTION_POINTERS* ep)
{
    char buf[64];
    wsprintfA(buf, "[FATAL] SEH code: 0x%08X\n", ep->ExceptionRecord->ExceptionCode);
    RawLog(buf);
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif // _WIN32

// ─────────────────────────────────────────────────────────────────────────────
static int RKengMain()
{
    try
    {
        RKeng::Engine engine;
        engine.Init();
        engine.Run();
        engine.Shutdown();
    }
    catch (const std::exception& e)
    {
        std::string msg = std::string("[FATAL] ") + e.what() + "\n";
        std::cerr << msg << std::flush;
        RawLog(msg.c_str());
        return 1;
    }
    catch (...)
    {
        std::cerr << "[FATAL] Unknown exception\n" << std::flush;
        RawLog("[FATAL] unknown exception (not std::exception)\n");
        return 1;
    }
    return 0;
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    // 1. Открыть лог-файл сырым Win32 — до всего остального
    g_logFile = CreateFileA("rkeng.log",
        GENERIC_WRITE, FILE_SHARE_READ, nullptr,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    RawLog("[main] WinMain start\n");

    // 2. VEH — сразу после открытия файла, до загрузки любых DLL
    AddVectoredExceptionHandler(1, VehCrashHandler);
    SetUnhandledExceptionFilter(CrashHandler);
    RawLog("[main] VEH registered\n");

    // 3. Консоль для интерактивной разработки
    AttachConsoleOutput();
    RawLog("[main] console attached\n");

    // 4. Всё остальное
    RawLog("[main] before RKengMain\n");
    int result = RKengMain();
    RawLog("[main] after RKengMain\n");

    RawLog("[main] WinMain exit\n");
    if (g_logFile != INVALID_HANDLE_VALUE)
        CloseHandle(g_logFile);
    return result;
}
#else
int main()
{
    return RKengMain();
}
#endif
