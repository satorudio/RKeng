#include "PlayerMove.h"
#include "../utils/Logger.h"
#include <algorithm>
#include <cmath>

#ifdef RK_JOLT_ENABLED
#include <Jolt/Physics/Character/CharacterVirtual.h>
#endif

namespace RKeng::PlayerMove
{
    // ── Мышь → yaw/pitch ────────────────────────────────────────────────────
    static void UpdateLook(InputState& input)
    {
        constexpr float SENSITIVITY = 0.1f;
        input.yaw   += input.mouseDeltaX * SENSITIVITY;
        input.pitch -= input.mouseDeltaY * SENSITIVITY;
        input.pitch  = std::clamp(input.pitch, -89.0f, 89.0f);
    }

    // ── Высота капсулы при приседе ───────────────────────────────────────────
    static void UpdateCrouchHeight(PlayerState& player, float dt)
    {
        float targetHeight = player.isCrouching
            ? player.crouchHeight
            : player.height;
        constexpr float SPEED = 10.0f;
        player.currentHeight += (targetHeight - player.currentHeight) * SPEED * dt;
    }

    // ── Сальто — двойной прыжок крутит pitch на 360° ───────────────────────
    static void UpdateSomersault(PlayerState& player, InputState& input, float dt)
    {
        constexpr float SOMERSAULT_SPEED = 360.0f; // градусов в секунду (1 сек на оборот)

        if (player.isSomersaulting)
        {
            float delta = SOMERSAULT_SPEED * dt;
            player.somersaultAngle += delta;

            // Применяем к pitch — вращаем вокруг горизонтальной оси
            input.pitch += delta;
            // pitch не clamped во время сальто — он летит свободно

            if (player.somersaultAngle >= 360.0f)
            {
                // Завершили оборот — возвращаем pitch в исходное (0 = горизонт)
                input.pitch = std::clamp(input.pitch - 360.0f, -89.0f, 89.0f);
                // snap к ближайшему кратному, чтобы не было рывка
                if (input.pitch > 89.0f)  input.pitch = 89.0f;
                if (input.pitch < -89.0f) input.pitch = -89.0f;
                player.isSomersaulting = false;
                player.somersaultAngle = 0.0f;
            }
        }

        // Разрешаем двойной прыжок сразу после первого прыжка (пока в воздухе)
        if (player.onGround)
        {
            player.canDoubleJump = true;
            player.isSomersaulting = false;
            player.somersaultAngle = 0.0f;
        }

        // Активация сальто: пробел нажат в воздухе, доступен двойной прыжок
        if (input.jumpPressed && !player.onGround && player.canDoubleJump && !player.isSomersaulting)
        {
            player.isSomersaulting = true;
            player.somersaultAngle = 0.0f;
            player.canDoubleJump   = false; // только одно сальто за прыжок
        }
    }

#ifdef RK_JOLT_ENABLED
    static JPH::Vec3 BuildWishVelocity(const InputState& input, const PlayerState& player)
    {
        float yawRad = input.yaw * RKeng::DEG2RAD;
        JPH::Vec3 forward( std::sin(yawRad), 0.0f, -std::cos(yawRad));
        JPH::Vec3 right  ( std::cos(yawRad), 0.0f,  std::sin(yawRad));

        JPH::Vec3 wish = JPH::Vec3::sZero();
        if (input.forward)  wish += forward;
        if (input.backward) wish -= forward;
        if (input.right)    wish += right;
        if (input.left)     wish -= right;

        float len = wish.Length();
        if (len > 1e-4f) wish = wish / len;

        float speed = player.isCrouching ? player.crouchSpeed
                    : player.isRunning   ? player.runSpeed
                    :                      player.walkSpeed;
        return wish * speed;
    }
#endif

    RK_API void Run(SceneState& scene, PhysicsState& ph)
    {
        const float dt = scene.deltaTime;
        auto& player   = scene.player;
        auto& input    = scene.input;

        UpdateLook(input);

        player.isCrouching = input.crouch;
        player.isRunning   = input.run && !input.crouch;
        UpdateCrouchHeight(player, dt);

#ifdef RK_JOLT_ENABLED
        if (!ph.initialized || !ph.character) return;

        auto* ch = ph.character.get();

        // onGround и позиция обновляются в PhysicsTick сразу после physicsSystem->Update.
        // Здесь только читаем состояние и задаём скорость для следующего шага.

        // Сальто обновляем до прыжка — он читает onGround и jumpPressed
        UpdateSomersault(player, input, dt);

        JPH::Vec3 curVel  = ch->GetLinearVelocity();
        JPH::Vec3 wishVel = BuildWishVelocity(input, player);
        JPH::Vec3 newVel(wishVel.GetX(), curVel.GetY(), wishVel.GetZ());

        // Первый прыжок — с земли
        if (input.jumpPressed && player.onGround)
            newVel.SetY(player.jumpImpulse);

        if (!player.onGround)
            newVel += ph.physicsSystem->GetGravity() * dt;

        ch->SetLinearVelocity(newVel);

        // Origin shift — обновляем на основе позиции, выставленной в PhysicsTick
        DVec3 localPos = player.worldPos.world - scene.originShift;
        double dist = glm::length(localPos);
        if (dist > ORIGIN_SHIFT_THRESHOLD)
            scene.originShift = player.worldPos.world;

#else
        // Без физики — движение + прыжок через простую вертикальную скорость
        static float s_velY = 0.0f;
        constexpr float GRAVITY    = -20.0f;
        constexpr float GROUND_Y   =  0.0f;

        float speed  = player.isRunning ? player.runSpeed : player.walkSpeed;
        float yawRad = input.yaw * DEG2RAD;

        if (input.forward) {
            player.worldPos.world.x -= std::sin(yawRad) * speed * dt;
            player.worldPos.world.z -= std::cos(yawRad) * speed * dt;
        }
        if (input.backward) {
            player.worldPos.world.x += std::sin(yawRad) * speed * dt;
            player.worldPos.world.z += std::cos(yawRad) * speed * dt;
        }
        if (input.left) {
            player.worldPos.world.x -= std::cos(yawRad) * speed * dt;
            player.worldPos.world.z += std::sin(yawRad) * speed * dt;
        }
        if (input.right) {
            player.worldPos.world.x += std::cos(yawRad) * speed * dt;
            player.worldPos.world.z -= std::sin(yawRad) * speed * dt;
        }

        bool onGround = (player.worldPos.world.y <= GROUND_Y + 0.001);
        if (input.jump && onGround)
            s_velY = player.jumpImpulse;

        if (!onGround)
            s_velY += GRAVITY * dt;
        else
            s_velY = 0.0f;

        player.worldPos.world.y += s_velY * dt;
        if (player.worldPos.world.y < GROUND_Y)
            player.worldPos.world.y = GROUND_Y;
#endif
    }
}
