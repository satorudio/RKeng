#pragma once
#include "types.h"

namespace RKeng {

    struct CharacterAPI {
        bool  (*CreateCharacter)   (RK_WorldHandle world, const RK_CharacterDesc& desc) = nullptr;
        void  (*SetPlayerVelocity) (RK_WorldHandle world, float vx, float vy, float vz) = nullptr;
        void  (*GetPlayerVelocity) (RK_WorldHandle world, float& vx, float& vy, float& vz) = nullptr;
        float (*GetGravityY)       (RK_WorldHandle world) = nullptr;
    };
}
