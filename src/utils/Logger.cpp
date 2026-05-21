#include "Logger.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <mutex>

// ── ANSI colour codes ─────────────────────────────────────────────────────
#define COL_RESET    "\033[0m"
#define COL_GREY     "\033[90m"
#define COL_CYAN     "\033[96m"
#define COL_GREEN    "\033[92m"
#define COL_YELLOW   "\033[93m"
#define COL_RED      "\033[91m"
#define COL_MAGENTA  "\033[95m"
#define COL_BLUE     "\033[94m"
#define COL_BOLD     "\033[1m"
#define COL_RED_BOLD "\033[1;91m"

namespace
{
    static std::mutex s_logMutex;
}

namespace RKeng::Logger
{
    static std::string Timestamp()
    {
        using namespace std::chrono;
        auto now  = system_clock::now();
        auto ms   = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
        auto t    = system_clock::to_time_t(now);
        std::tm bt{};
#ifdef _WIN32
        localtime_s(&bt, &t);
#else
        localtime_r(&t, &bt);
#endif
        std::ostringstream oss;
        oss << COL_GREY
            << std::setfill('0')
            << std::setw(2) << bt.tm_hour << ':'
            << std::setw(2) << bt.tm_min  << ':'
            << std::setw(2) << bt.tm_sec  << '.'
            << std::setw(3) << ms.count()
            << COL_RESET << ' ';
        return oss.str();
    }

    void Init()
    {
        std::lock_guard<std::mutex> lock(s_logMutex);
#ifdef _WIN32
        // Включаем ANSI escape codes в cmd.exe / PowerShell

        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);
        DWORD mode = 0;
        if (GetConsoleMode(hOut, &mode))
            SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        if (GetConsoleMode(hErr, &mode))
            SetConsoleMode(hErr, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
        std::cout << Timestamp()
                  << COL_BOLD << COL_CYAN << "[RKeng]" << COL_RESET
                  << " Logger ready.\n" << std::flush;
    }

    void Shutdown()
    {
        std::lock_guard<std::mutex> lock(s_logMutex);
        std::cout << Timestamp()
                  << COL_BOLD << COL_CYAN << "[RKeng]" << COL_RESET
                  << " Logger closed.\n" << std::flush;
    }

    void Info(std::string_view msg)
    {
        std::lock_guard<std::mutex> lock(s_logMutex);
        std::cout << Timestamp()
                  << COL_GREEN << "[INFO] " << COL_RESET
                  << msg << '\n' << std::flush;
    }

    void Warn(std::string_view msg)
    {
        std::lock_guard<std::mutex> lock(s_logMutex);
        std::cout << Timestamp()
                  << COL_YELLOW << "[WARN] " << COL_RESET
                  << COL_YELLOW << msg << COL_RESET
                  << '\n' << std::flush;
    }

    void Error(std::string_view msg)
    {
        std::lock_guard<std::mutex> lock(s_logMutex);
        std::cerr << Timestamp()
                  << COL_RED << "[ERROR]" << COL_RESET
                  << ' ' << COL_RED << msg << COL_RESET
                  << '\n' << std::flush;
    }

    void Fatal(std::string_view msg)
    {
        std::lock_guard<std::mutex> lock(s_logMutex);
        std::cerr << Timestamp()
                  << COL_RED_BOLD << "[FATAL]" << COL_RESET
                  << ' ' << COL_RED_BOLD << msg << COL_RESET
                  << '\n' << std::flush;
    }

    void Debug(std::string_view msg)
    {
#ifdef RK_DEBUG
        std::lock_guard<std::mutex> lock(s_logMutex);
        std::cout << Timestamp()
                  << COL_GREY << "[DEBUG] " << msg << COL_RESET
                  << '\n' << std::flush;
#else
        (void)msg;
#endif
    }

    void Trace(std::string_view msg)
    {
#ifdef RK_DEBUG
        std::lock_guard<std::mutex> lock(s_logMutex);
        std::cout << Timestamp()
                  << COL_GREY << "[TRACE] " << msg << COL_RESET
                  << '\n' << std::flush;
#else
        (void)msg;
#endif
    }

    // Диагностика указателей — всегда активна (не только в DEBUG).
    // Формат:  [PTR]  JPH::Allocate        = 0x00007ff84a3c1020  OK
    //          [PTR]  JPH::Factory::sInst  = 0x0000000000000000  <<< NULL !!!
    void Ptr(std::string_view name, const void* ptr)
    {
        std::ostringstream oss;
        oss << std::hex << std::uppercase << std::setfill('0')
            << "0x" << std::setw(16) << reinterpret_cast<uintptr_t>(ptr);

        const bool null = (ptr == nullptr);
        std::lock_guard<std::mutex> lock(s_logMutex);
        std::cout << Timestamp()
                  << COL_BLUE << "[PTR]  " << COL_RESET
                  << std::left << std::setw(32) << name
                  << (null ? COL_RED_BOLD : COL_GREEN)
                  << oss.str()
                  << (null ? "  <<< NULL !!!" : "  OK")
                  << COL_RESET
                  << '\n' << std::flush;
    }

} // namespace RKeng::Logger
