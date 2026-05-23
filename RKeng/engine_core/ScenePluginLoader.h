#pragma once
// ScenePluginLoader.h — загружает и выгружает DLL со сценой.
// Живёт в движке. Сцена про него не знает.
//
// Использование в EngineInit:
//   ScenePluginLoader loader;
//   loader.Load("VoxelCarWorld.dll");          // или .so на Linux
//   loader.GetPlugin()->OnLoad(scene, physics, api);
//
//   // При смене уровня:
//   loader.Unload();
//   loader.Load("NextLevel.dll");

#include "IScenePlugin.h"
#include <string>
#include <stdexcept>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
   using DylibHandle = HMODULE;
#  define RK_DLOPEN(p)    LoadLibraryA(p)
#  define RK_DLSYM(h, s)  GetProcAddress(h, s)
#  define RK_DLCLOSE(h)   FreeLibrary(h)
#  define RK_DLERROR()    std::to_string(GetLastError())
#else
#  include <dlfcn.h>
   using DylibHandle = void*;
#  define RK_DLOPEN(p)    dlopen(p, RTLD_NOW | RTLD_LOCAL)
#  define RK_DLSYM(h, s)  dlsym(h, s)
#  define RK_DLCLOSE(h)   dlclose(h)
#  define RK_DLERROR()    std::string(dlerror())
#endif

namespace RKeng
{
    class ScenePluginLoader
    {
    public:
        ScenePluginLoader() = default;
        ~ScenePluginLoader() { Unload(); }

        // Некопируемый — владеет хэндлом DLL
        ScenePluginLoader(const ScenePluginLoader&) = delete;
        ScenePluginLoader& operator=(const ScenePluginLoader&) = delete;

        // Загружает DLL и создаёт экземпляр сцены через RK_CreateScene().
        // Бросает std::runtime_error если что-то не так.
        void Load(const std::string& dllPath)
        {
            Unload(); // на случай если уже что-то загружено

            m_Handle = RK_DLOPEN(dllPath.c_str());
            if (!m_Handle)
                throw std::runtime_error(
                    "[ScenePluginLoader] Cannot load '" + dllPath + "': " + RK_DLERROR());

            m_CreateFn  = reinterpret_cast<CreateSceneFn >(RK_DLSYM(m_Handle, "RK_CreateScene"));
            m_DestroyFn = reinterpret_cast<DestroySceneFn>(RK_DLSYM(m_Handle, "RK_DestroyScene"));

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

        // Выгружает текущую сцену и FreeLibrary.
        void Unload()
        {
            if (m_Plugin && m_DestroyFn)
            {
                m_DestroyFn(m_Plugin);
                m_Plugin = nullptr;
            }
            if (m_Handle)
            {
                RK_DLCLOSE(m_Handle);
                m_Handle = nullptr;
            }
            m_DllPath.clear();
        }

        // Возвращает nullptr если ничего не загружено
        IScenePlugin* GetPlugin() const { return m_Plugin; }
        bool          IsLoaded()  const { return m_Plugin != nullptr; }
        const std::string& GetPath() const { return m_DllPath; }

    private:
        DylibHandle    m_Handle    = nullptr;
        IScenePlugin*  m_Plugin    = nullptr;
        CreateSceneFn  m_CreateFn  = nullptr;
        DestroySceneFn m_DestroyFn = nullptr;
        std::string    m_DllPath;
    };
}
