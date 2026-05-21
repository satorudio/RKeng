#include "ServerState.h"

namespace RKeng::Server
{
    ServerState& Get()
    {
        static ServerState s;
        return s;
    }
}
