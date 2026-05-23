#pragma once
// CarState.h — параметры RAM 2500 Power Wagon и рабочий стейт машины.
//
// Реальные характеристики Power Wagon (6.4L HEMI, crew cab, 4WD):
//   Масса снаряжённая : ~3 493 кг
//   База              : 3 569 мм → ~3.57 м
//   Ширина / высота   : 2.01 / 1.93 м
//   Клиренс           : 292 мм
//   Угол въезда/съезда: 44 / 26 °
//   Ход подвески      : ~230 мм (front coil, rear leaf-spring адаптируем к coil)

#include "MathTypes.h"
#include <vector>
#include <array>
#include <cstdint>

#ifdef RK_JOLT_ENABLED
#  include <Jolt/Jolt.h>
#  include <Jolt/Physics/Body/BodyID.h>
// ВАЖНО: VehicleConstraint.h и WheeledVehicleController.h НЕ включаем здесь!
// Они содержат JPH_IMPLEMENT_RTTI_VIRTUAL — глобальные объекты с конструкторами,
// которые инициализируются при загрузке DLL (до DllMain), обращаясь к
// JPH::Factory::sInstance который в тот момент ещё nullptr → EXCEPTION_BREAKPOINT.
// Включай только в .cpp файлах, после InitJoltFromEngine().
namespace JPH { class VehicleConstraint; template<class T> class Ref; }
#endif

namespace RKeng
{
    // ── Управляющие входы ────────────────────────────────────────────────────
    struct CarInput
    {
        float throttle  = 0.0f;  // 0..1   (W)
        float brake     = 0.0f;  // 0..1   (S)
        float steer     = 0.0f;  // -1..1  (A / D)
        bool  handbrake = false; // Space
        bool  lowRange  = false; // L — пониженная передача (снижает maxRPM→maxTorque)
    };

    // ── Параметры грязевой буксовки ──────────────────────────────────────────
    struct MudParams
    {
        // Коэффициент снижения фрикции в грязи
        float frictionMul   = 0.28f;
        // Сила дополнительного сопротивления (Н) на каждое колесо в грязи
        float dragForcePerWheel = 900.0f;
        // Шанс визуального спинапа колёс (частицы)
        float spinTreshold  = 0.55f;   // throttle > этого в грязи → буксуем
    };

    // ── Физические параметры кузова и ходовой ───────────────────────────────
    struct CarPhysicsParams
    {
        // --- Кузов (half-extents в метрах) ---
        // Power Wagon: 2.01m wide × 1.93m tall × 5.92m long
        float halfW     = 1.005f;  // по X — ширина
        float halfH     = 0.48f;   // по Y — высота кузовного коллайдера (без рамы)
        float halfL     = 2.96f;   // по Z — длина (wheelbase + свесы)
        float mass      = 3493.0f; // кг

        float linearDamping  = 0.08f;
        float angularDamping = 0.65f;
        float friction       = 0.25f;  // у кузова низкий — фрикция у колёс

        // --- Подвеска (Double-A-arm front / 5-link rear, ход 230мм) ---
        float suspMinLen    = 0.05f;
        float suspMaxLen    = 0.28f;   // 230 мм ≈ 0.23, даём небольшой запас
        float suspFreq      = 1.6f;    // Hz — мягкая внедорожная подвеска
        float suspDamping   = 0.45f;

        // --- Колёса (LT275/70R18 по умолч.) ---
        // R18 + sidewall: (18*25.4/2 + 275*0.70)/1000 ≈ 0.421 м
        float wheelRadius   = 0.421f;
        float wheelWidth    = 0.165f;   // ≈ 275mm/1000*0.6 (visual width)

        // --- Рулёжка ---
        float maxSteerDeg   = 35.0f;   // Power Wagon имеет широкий поворот

        // --- Двигатель (6.4L HEMI 410 л.с. / 644 Н·м при 4000 об/мин) ---
        float maxTorque     = 644.0f;
        float maxRPM        = 5600.0f;
        float engineInertia = 1.4f;

        // --- Трансмиссия ---
        // 8HP75 8AT: передаточные числа 1-й..8-й + задний ход
        // Упрощаем до тех что важны для off-road
        float gearRatios[8] = { 4.71f, 3.14f, 2.10f, 1.67f,
                                 1.29f, 1.00f, 0.84f, 0.67f };
        float revGearRatio  = -3.30f;
        float transferLow   = 2.64f;  // пониженная 4L
        float switchTime    = 0.40f;

        // --- Дифференциалы (4WD, оба locked при lowRange) ---
        float diffSlipRatio     = 1.4f;
        float diffSlipRatioLock = 1.01f;  // при lowRange — почти блокировка

        // --- Тормоза ---
        float handbrakeForce    = 10000.0f;

        // --- Антикрен ---
        float antiRollFront     = 2000.0f;
        float antiRollRear      = 1800.0f;

        // --- Фрикция колёс ---
        float frontFriction     = 1.8f;
        float rearFriction      = 1.8f;
    };

    // ── Damage /HP ──────────────────────────────────────────────────────────
    struct CarDamage
    {
        float totalHP           = 200.0f;
        float currentHP         = 200.0f;
        bool  destroyed         = false;
        float voxelBreakImpulse = 1200.0f;
        float voxelCrackImpulse = 500.0f;
    };

    // ── Состояние в грязи ────────────────────────────────────────────────────
    struct MudState
    {
        bool  inMud            = false;
        int   wheelsInMud      = 0;    // 0..4
        float mudDepth         = 0.0f; // 0..1 — визуальная вязкость
        float spinParticleTimer = 0.0f;
    };

    // ── Воксели (визуальный кузов, не физика) ───────────────────────────────
    // RAM 2500 — представляем в вокселях 0.18м для детализации
    constexpr float CAR_VOXEL_SIZE = 0.18f;
    constexpr int   CAR_VX_W       = 11;   // ≈ 2.0 м ширина
    constexpr int   CAR_VX_H       = 10;   // ≈ 1.8 м высота
    constexpr int   CAR_VX_L       = 33;   // ≈ 5.9 м длина

    struct CarVoxel
    {
        bool  alive  = true;
        Vec3  color  { 0.12f, 0.22f, 0.45f };  // RAM синий
        float health = 1.0f;
    };

    struct CarDebris
    {
        Vec3  pos;
        Vec3  velocity;
        Vec3  color;
        float size;
        float lifetime = 0.0f;
        bool  dead     = false;
    };

    // ── Главный стейт ────────────────────────────────────────────────────────
    struct CarState
    {
        CarInput        input;
        CarPhysicsParams params;
        CarDamage       damage;
        MudParams       mudParams;
        MudState        mud;

        // Визуальный воксельный кузов
        CarVoxel voxels[CAR_VX_W][CAR_VX_H][CAR_VX_L];
        bool     meshDirty = true;

        std::vector<CarDebris> debris;

        // Мировой трансформ (синхронизируется из Jolt каждый тик)
        Vec3  position    { 0.0f, 2.5f, 0.0f };
        Quat  orientation { 1.0f, 0.0f, 0.0f, 0.0f };
        Vec3  velocity    { 0.0f, 0.0f, 0.0f };
        float speedKph    = 0.0f;
        int   currentGear = 1;

        // Камера
        float camYaw        = 0.0f;
        float camPitch      = 0.0f;
        Vec3  camLocalOffset { 0.0f, 2.4f, -5.5f };  // вид сзади-сверху

        // GPU меш
        std::vector<float>    meshVertices;  // pos[3]+color[3]+normal[3] = 9 floats
        std::vector<uint32_t> meshIndices;

#ifdef RK_JOLT_ENABLED
        JPH::BodyID          bodyID;
        // Используем сырой указатель вместо JPH::Ref<> — иначе включение
        // VehicleConstraint.h в заголовок вызывает RTTI-регистрацию при
        // загрузке DLL (до InitJoltFromEngine) → Factory nullptr → краш.
        // Владение: CarLoad::Run() ставит AddRef вручную,
        //            CarLoad::Destroy() вызывает Release вручную.
        JPH::VehicleConstraint* vehicleConstraint = nullptr;
#endif

        bool initialized = false;
    };

    // Алиасы для обратной совместимости с CarMesh.cpp движка,
    // который использует старые имена констант
    constexpr int CAR_VOXELS_W = CAR_VX_W;
    constexpr int CAR_VOXELS_H = CAR_VX_H;
    constexpr int CAR_VOXELS_L = CAR_VX_L;

}  // namespace RKeng
