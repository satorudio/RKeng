#include "SceneState.h"

namespace RKeng
{
    SceneState& GetSceneState()
    {
        static SceneState s;
        return s;
    }
}
