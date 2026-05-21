#pragma once
#ifdef RK_JOLT_ENABLED
#include <Jolt/Physics/Collision/ContactListener.h>
#include <functional>

namespace RKeng {

// Вызывается физическим движком при реальном контакте тел.
// Регистрируется в PhysicsSystem один раз при инициализации.
class RKContactListener final : public JPH::ContactListener
{
public:
    // bodyID1, bodyID2 — участники столкновения
    // contactPoint    — точка контакта в мировых координатах
    // impulse         — оценка импульса (penetrationDepth * hardness)
    using HitCallback = std::function<void(
        JPH::BodyID,   // body1
        JPH::BodyID,   // body2
        JPH::Vec3,     // contactPoint (world space)
        float          // impulse
    )>;

    void SetHitCallback(HitCallback cb) { m_cb = std::move(cb); }

    void OnContactAdded(const JPH::Body&           b1,
                        const JPH::Body&           b2,
                        const JPH::ContactManifold& manifold,
                        JPH::ContactSettings&) override
    {
        if (!m_cb) return;

        // penetrationDepth — достаточно хорошее приближение импульса для UI/урона.
        // При необходимости замени на mCombinedRestitution * relative_velocity.
        float impulse = manifold.mPenetrationDepth;

        m_cb(b1.GetID(),
             b2.GetID(),
             manifold.GetWorldSpaceContactPointOn1(0),
             impulse);
    }

private:
    HitCallback m_cb;
};

} // namespace RKeng
#endif // RK_JOLT_ENABLED
