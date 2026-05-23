#pragma once
// ScenePluginLoader.h — загружает/выгружает DLL со сценой.
// Живёт только в движке. Сцена про него не знает.

#include "../../engine_api/IScenePlugin.h"
#include "../utils/Logger.h"
#include <string>
#include <stdexcept>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
   using DylibHandle = HMODULE;
#  define RK_DLSYM(h, s) GetProcAddress(h, s)
#  define RK_DLCLOSE(h)  FreeLibrary(h)
#else
#  include <dlfcn.h>
   using DylibHandle = void*;
#  define RK_DLOPEN(p)   dlopen(p, RTLD_NOW | RTLD_LOCAL)
#  define RK_DLSYM(h, s) dlsym(h, s)
#  define RK_DLCLOSE(h)  dlclose(h)
#  define RK_DLERROR()   std::string(dlerror())
#endif

namespace RKeng
{
#ifdef _WIN32
    // Возвращает директорию где лежит текущий exe (с trailing slash)
    inline std::string GetExeDir()
    {
        char buf[MAX_PATH];
        DWORD len = GetModuleFileNameA(nullptr, buf, MAX_PATH);
        if (len == 0) return "";
        std::string path(buf, len);
        auto pos = path.find_last_of("\\/");
        return (pos != std::string::npos) ? path.substr(0, pos + 1) : "";
    }

    // Загружает DLL — сначала ищет рядом с exe, потом стандартный поиск
    inline DylibHandle RK_DLOPEN(const std::string& name)
    {
        // Если передан относительный/plain путь — пробуем рядом с exe явно
        bool hasSlash = name.find('/') != std::string::npos ||
                        name.find('\\') != std::string::npos;
        if (!hasSlash)
        {
            std::string fullPath = GetExeDir() + name;
            HMODULE h = LoadLibraryA(fullPath.c_str());
            if (h) return h;
        }
        return LoadLibraryA(name.c_str());
    }

    inline std::string RK_DLERROR()
    {
        DWORD code = GetLastError();
        char msgBuf[512] = {};
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       nullptr, code, 0, msgBuf, sizeof(msgBuf), nullptr);
        // Убираем trailing \r\n
        std::string msg(msgBuf);
        while (!msg.empty() && (msg.back() == '\r' || msg.back() == '\n' || msg.back() == ' '))
            msg.pop_back();
        return "error " + std::to_string(code) + ": " + msg;
    }
#endif

    class ScenePluginLoader
    {
    public:
        ScenePluginLoader() = default;
        ~ScenePluginLoader() { Unload(); }

        ScenePluginLoader(const ScenePluginLoader&) = delete;
        ScenePluginLoader& operator=(const ScenePluginLoader&) = delete;

        // Загружает DLL и создаёт экземпляр сцены через RK_CreateScene().
        // Бросает std::runtime_error если что-то не так.
        void Load(const std::string& dllPath)
        {
            Unload();

            Logger::Info("[Loader] LoadLibraryA start: " + dllPath);
            // Если крэш здесь (0x80000003/0xC0000005) — падение в статических
            // конструкторах DLL (JPH_IMPLEMENT_RTTI_VIRTUAL и т.п.) до DllMain.
#ifdef _WIN32
            m_Handle = RK_DLOPEN(dllPath);
#else
            m_Handle = RK_DLOPEN(dllPath.c_str());
#endif
            if (!m_Handle)
                throw std::runtime_error(
                    "[ScenePluginLoader] Cannot load '" + dllPath + "': " + RK_DLERROR());
            Logger::Info("[Loader] LoadLibraryA OK");

            // Двойной каст через void* — единственный способ беззвучно
            // конвертировать FARPROC (Win32) / void* (POSIX) в конкретный
            // тип функции без -Wcast-function-type. Стандарт это разрешает
            // (C++11 [expr.reinterpret.cast] p8) при условии что сигнатура
            // совпадает с реально экспортируемой функцией.
            void* rawCreate  = reinterpret_cast<void*>(RK_DLSYM(m_Handle, "RK_CreateScene"));
            void* rawDestroy = reinterpret_cast<void*>(RK_DLSYM(m_Handle, "RK_DestroyScene"));
            m_CreateFn  = reinterpret_cast<CreateSceneFn >(rawCreate);
            m_DestroyFn = reinterpret_cast<DestroySceneFn>(rawDestroy);

            if (!m_CreateFn || !m_DestroyFn)
            {
                RK_DLCLOSE(m_Handle);
                m_Handle = nullptr;
                throw std::runtime_error(
                    "[ScenePluginLoader] '" + dllPath +
                    "' не экспортирует RK_CreateScene / RK_DestroyScene");
            }

            m_Plugin  = m_CreateFn();
            m_DllPath = dllPath;
        }

        void Unload()
        {
            if (m_Plugin && m_DestroyFn)
            {
                m_DestroyFn(m_Plugin);
                m_Plugin    = nullptr;
                m_DestroyFn = nullptr;
                m_CreateFn  = nullptr;
            }
            if (m_Handle)
            {
                RK_DLCLOSE(m_Handle);
                m_Handle = nullptr;
            }
            m_DllPath.clear();
        }

        IScenePlugin*      GetPlugin() const { return m_Plugin; }
        bool               IsLoaded()  const { return m_Plugin != nullptr; }
        const std::string& GetPath()   const { return m_DllPath; }

    private:
        DylibHandle    m_Handle    = nullptr;
        IScenePlugin*  m_Plugin    = nullptr;
        CreateSceneFn  m_CreateFn  = nullptr;
        DestroySceneFn m_DestroyFn = nullptr;
        std::string    m_DllPath;
    };

} // namespace RKeng
