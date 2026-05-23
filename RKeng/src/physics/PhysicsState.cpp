#include "PhysicsState.h"
#ifdef RK_JOLT_ENABLED
// Полный тип нужен для unique_ptr<CharacterVirtual>::~unique_ptr()
// (деструктор инстанцируется в .cpp где живёт статический PhysicsState)
#include <Jolt/Physics/Character/CharacterVirtual.h>
#endif
namespace RKeng { PhysicsState& GetPhysicsState() { static PhysicsState s; return s; } }
