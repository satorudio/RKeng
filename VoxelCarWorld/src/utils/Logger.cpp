#include "Logger.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <ctime>

// ── ANSI colour codes (работают в MINGW/ConEmu/Windows Terminal) ──────────
#define COL_RESET   "\033[0m"
#define COL_GREY    "\033[90m"
#define COL_CYAN    "\033[96m"
#define COL_GREEN   "\033[92m"
#define COL_YELLOW  "\033[93m"
#define COL_RED     "\033[91m"
#define COL_MAGENTA "\033[95m"
#define COL_BOLD    "\033[1m"
#define COL_RED_BOLD "\033[1;91m"

namespace RKeng::Logger
{
    // ── Timestamp HH:MM:SS.mmm ───────────────────────────────────────────
    static std::string Timestamp()
    {
        using namespace std::chrono;
        auto now   = system_clock::now();
        auto ms    = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
        auto timer = system_clock::to_time_t(now);
        std::tm bt{};
#ifdef _WIN32
        localtime_s(&bt, &timer);
#else
        localtime_r(&timer, &bt);
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
        // Включаем ANSI в Windows-терминале
#ifdef _WIN32
        // Для MINGW stdout/stderr уже поддерживают ANSI если запускать в Windows Terminal
        // SetConsoleMode нужен только для cmd.exe — пропускаем, не ломаем MINGW
#endif
        std::cout << Timestamp()
                  << COL_BOLD << COL_CYAN << "[RKeng]" << COL_RESET
                  << " Logger ready.\n" << std::flush;
    }

    void Shutdown()
    {
        std::cout << Timestamp()
                  << COL_BOLD << COL_CYAN << "[RKeng]" << COL_RESET
                  << " Logger closed.\n" << std::flush;
    }

    void Info(std::string_view msg)
    {
        std::cout << Timestamp()
                  << COL_GREEN << "[INFO] " << COL_RESET
                  << msg << '\n' << std::flush;
    }

    void Warn(std::string_view msg)
    {
        std::cout << Timestamp()
                  << COL_YELLOW << "[WARN] " << COL_RESET
                  << COL_YELLOW << msg << COL_RESET
                  << '\n' << std::flush;
    }

    void Error(std::string_view msg)
    {
        std::cerr << Timestamp()
                  << COL_RED << "[ERROR]" << COL_RESET
                  << ' ' << COL_RED << msg << COL_RESET
                  << '\n' << std::flush;
    }

    void Fatal(std::string_view msg)
    {
        std::cerr << Timestamp()
                  << COL_RED_BOLD << "[FATAL]" << COL_RESET
                  << ' ' << COL_RED_BOLD << msg << COL_RESET
                  << '\n' << std::flush;
    }

    void Debug(std::string_view msg)
    {
#ifdef RK_DEBUG
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
        std::cout << Timestamp()
                  << COL_GREY << "[TRACE] " << msg << COL_RESET
                  << '\n' << std::flush;
#else
        (void)msg;
#endif
    }
}
