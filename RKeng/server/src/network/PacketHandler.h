#pragma once
#include "../core/ServerState.h"
#include "Protocol.h"
#include <cstddef>

namespace RKeng::Server::PacketHandler
{
    void Dispatch(ServerState& srv,
                  Net::Addr            from,
                  Protocol::Header*    hdr,
                  const void*          data,
                  size_t               size);
}
