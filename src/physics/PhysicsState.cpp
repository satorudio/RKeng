#include "PhysicsState.h"
namespace RKeng { PhysicsState& GetPhysicsState() { static PhysicsState s; return s; } }
