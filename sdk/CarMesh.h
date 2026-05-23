#pragma once
#include "CarState.h"

namespace RKeng::CarMesh
{
    // Перестроить меш из живых вокселей + дебрис
    void Rebuild(CarState& car);
}
