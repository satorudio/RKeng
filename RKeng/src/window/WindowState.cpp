#include "WindowState.h"

namespace RKeng
{
    WindowState& GetWindowState()
    {
        static WindowState s;
        return s;
    }
}
