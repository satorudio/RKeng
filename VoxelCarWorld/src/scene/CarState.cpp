#include "scene/CarState.h"

namespace RKeng
{
    static CarState s_car;
    CarState& GetCarState() { return s_car; }
}
