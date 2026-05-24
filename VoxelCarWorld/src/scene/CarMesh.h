#pragma once
#include "CarState.h"

namespace RKeng::CarMesh
{
    // Строит статичный box-меш по размерам params.
    // Вызывается один раз в OnLoad — меш не меняется.
    void Build(CarState& car);
}
