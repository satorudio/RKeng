#pragma once
// JoltBridge.h — пробрасывает синглтоны Jolt из движка в DLL-сцену.
//
// ПРОБЛЕМА:
//   Движок и DLL-сцена оба статически линкуют libJolt.a.
//   Значит у каждого своя копия глобальных переменных:
//     JPH::Allocate, JPH::Free, JPH::AlignedAllocate, JPH::Factory::sInstance, ...
//   Движок инициализирует СВОИ копии в PhysicsInit::Run().
//   Копии в DLL остаются nullptr — первый же JPH::new / BoxShapeSettings::Create()
//   обращается к nullptr → access violation 0xc0000005.
//
// РЕШЕНИЕ:
//   Движок передаёт все указатели через EngineAPI (поля joltAllocate* / joltFactory).
//   DLL вызывает InitJoltFromEngine(api) в самом начале OnLoad() —
//   до любого вызова Jolt-кода.
//
// ИСПОЛЬЗОВАНИЕ в OnLoad() DLL:
//
//   #include <sdk/JoltBridge.h>
//
//   void MyScene::OnLoad(SceneState& scene, PhysicsState& ph, const EngineAPI& api)
//   {
//   #ifdef RK_JOLT_ENABLED
//       RKeng::InitJoltFromEngine(api);   // ← ПЕРВАЯ строка, до любого JPH::
//   #endif
//       // ... дальше можно использовать Jolt нормально
//       WorldGen::Generate(scene, ph);
//   }
//
// ВАЖНО:
//   - НЕ вызывай JPH::RegisterDefaultAllocator() в DLL — это перезапишет указатели.
//   - НЕ вызывай new JPH::Factory() в DLL — фабрика должна быть одна, из движка.
//   - JPH::RegisterTypes() вызывать можно — он идемпотентен при общей фабрике.

#ifdef RK_JOLT_ENABLED

#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/Memory.h>
#include <Jolt/Core/IssueReporting.h>

#include "EngineAPI.h"

namespace RKeng
{
    // Пробрасывает все Jolt-синглтоны из движка в эту DLL.
    // Вызывай один раз — в самом начале OnLoad(), до любого кода Jolt.
    inline void InitJoltFromEngine(const EngineAPI& api)
    {
        // ── Аллокаторы ───────────────────────────────────────────────────
        // Без них первый же JPH::Allocate() → nullptr → 0xc0000005
        if (api.joltAllocate)
            JPH::Allocate        = reinterpret_cast<JPH::AllocateFunction        >(api.joltAllocate);
        if (api.joltFree)
            JPH::Free            = reinterpret_cast<JPH::FreeFunction            >(api.joltFree);
        if (api.joltReallocate)
            JPH::Reallocate      = reinterpret_cast<JPH::ReallocateFunction      >(api.joltReallocate);
        if (api.joltAllocate16)
            JPH::AlignedAllocate = reinterpret_cast<JPH::AlignedAllocateFunction >(api.joltAllocate16);
        if (api.joltFree16)
            JPH::AlignedFree     = reinterpret_cast<JPH::AlignedFreeFunction     >(api.joltFree16);

        // ── Factory ──────────────────────────────────────────────────────
        // Нужна для BoxShapeSettings::Create() и вообще любой Shape.
        if (api.joltFactory)
            JPH::Factory::sInstance = reinterpret_cast<JPH::Factory*>(api.joltFactory);

        // ── Assert handler ───────────────────────────────────────────────
        // Опционально, но без него Jolt-ассерты молча падают.
        if (api.joltAssertFn)
            JPH::AssertFailed = reinterpret_cast<decltype(JPH::AssertFailed)>(api.joltAssertFn);
    }

} // namespace RKeng

#endif // RK_JOLT_ENABLED
