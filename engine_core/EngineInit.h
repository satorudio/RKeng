#pragma once
#include "../scene/ScenePluginLoader.h"

namespace RKeng::EngineInit { void Run(bool& outRunning); }

// Доступ к загрузчику сцены — нужен Shutdown и потенциально горячей перезагрузке
namespace RKeng { ScenePluginLoader& GetSceneLoader(); }
