#include "SceneLoad.h"
#include "WorldGen.h"
#include "CarLoad.h"
#include "../utils/Logger.h"

#ifdef RK_JOLT_ENABLED
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#endif

namespace RKeng::SceneLoad
{
    void Run(SceneState& scene, PhysicsState& ph)
    {
#ifdef RK_JOLT_ENABLED
        if (!ph.initialized) return;

        // Генерируем мир (пол + барьеры + рандомные препятствия)
        WorldGen::Generate(scene, ph);

        // Спавним машину
        CarLoad::Run(GetCarState(), ph, Vec3(0.0f, 1.5f, 0.0f));

        // ОБЯЗАТЕЛЬНО после добавления всех статических тел
        ph.physicsSystem->OptimizeBroadPhase();

        // VehicleConstraint регистрируем ПОСЛЕ OptimizeBroadPhase
        {
            auto& car = GetCarState();
            if (car.vehicleConstraint != nullptr) {
                Logger::Info("SceneLoad: registering VehicleConstraint...");
                ph.physicsSystem->AddConstraint(car.vehicleConstraint);
                ph.physicsSystem->AddStepListener(car.vehicleConstraint);
                Logger::Info("SceneLoad: VehicleConstraint registered OK.");
            }
        }

        // CharacterVirtual создаём ПОСЛЕ OptimizeBroadPhase
        Logger::Info("SceneLoad: creating CharacterVirtual...");
        ph.characterSettings.mMaxSlopeAngle             = JPH::DegreesToRadians(45.0f);
        ph.characterSettings.mMaxStrength               = 100.0f;
        ph.characterSettings.mBackFaceMode              = JPH::EBackFaceMode::CollideWithBackFaces;
        ph.characterSettings.mCharacterPadding          = 0.02f;
        ph.characterSettings.mPenetrationRecoverySpeed  = 1.0f;
        ph.characterSettings.mPredictiveContactDistance = 0.1f;

        // ИСПРАВЛЕНИЕ 1: создаём CapsuleShape через ShapeSettings, а не через new напрямую.
        // Параметры: half-height цилиндрической части = 0.9f, radius = 0.35f.
        // Итоговая высота капсулы = 2*(halfHeight + radius) = 2*1.25 = 2.5 м.
        const float capsuleHalfHeight = 0.9f;
        const float capsuleRadius     = 0.35f;

        JPH::CapsuleShapeSettings capsuleSettings(capsuleHalfHeight, capsuleRadius);
        auto capsuleResult = capsuleSettings.Create();
        if (capsuleResult.HasError()) {
            Logger::Error(std::string("SceneLoad: CapsuleShape error: ") +
                          capsuleResult.GetError().c_str());
            return;
        }

        // ИСПРАВЛЕНИЕ 2: CharacterVirtual считает origin персонажа нижней точкой капсулы
        // (точка на полу). Поэтому оборачиваем капсулу в RotatedTranslatedShape и
        // смещаем её вверх на (halfHeight + radius), чтобы низ капсулы совпадал с origin.
        JPH::RotatedTranslatedShapeSettings rtsSettings(
            JPH::Vec3(0.0f, capsuleHalfHeight + capsuleRadius, 0.0f),
            JPH::Quat::sIdentity(),
            capsuleResult.Get());
        auto rtsResult = rtsSettings.Create();
        if (rtsResult.HasError()) {
            Logger::Error("SceneLoad: RotatedTranslatedShape error");
            return;
        }

        ph.characterSettings.mShape = rtsResult.Get();

        ph.character = std::make_unique<JPH::CharacterVirtual>(
            &ph.characterSettings,
            JPH::RVec3(0.0f, 2.0f, 0.0f),
            JPH::Quat::sIdentity(),
            ph.physicsSystem.get());

        Logger::Info("Scene loaded: open world + car + character.");
#else
        WorldGen::Generate(scene, ph);
        CarLoad::Run(GetCarState(), ph, Vec3(0.0f, 1.5f, 0.0f));
        (void)ph;
#endif
    }
}