#pragma once
#include <cstdint>
#include <cstring>

// Protocol.h — единственное место где определены все пакеты.
// Клиент и сервер включают ОДИН этот файл.
// Правило: клиент НИКОГДА не доверяет своим же данным без подтверждения сервера.

namespace RKeng::Protocol
{
    // ── Версия протокола ──────────────────────────────────────────────────
    // При изменении структуры пакетов — инкрементируй.
    // Сервер отклоняет клиентов с другой версией.
    static constexpr uint16_t VERSION = 1;

    // ── Каналы ENet ───────────────────────────────────────────────────────
    static constexpr uint8_t CH_RELIABLE   = 0;  // важные события (спавн, смерть, строительство)
    static constexpr uint8_t CH_UNRELIABLE = 1;  // позиции, анимации — можно терять

    // ── Типы пакетов ──────────────────────────────────────────────────────
    enum class PacketType : uint8_t
    {
        // C→S
        C_Hello         = 0x01,  // подключение + версия протокола
        C_InputState    = 0x02,  // ввод игрока (позиция НЕ доверяется — только input)
        C_Disconnect    = 0x03,

        // S→C
        S_Welcome       = 0x10,  // подтверждение подключения + playerID
        S_Reject        = 0x11,  // отказ (версия, бан, etc.)
        S_WorldState    = 0x12,  // авторитетное состояние мира (позиции всех)
        S_PlayerJoined  = 0x13,
        S_PlayerLeft    = 0x14,
        S_Event         = 0x15,  // игровое событие (урон, смерть, etc.)
    };

    // ── Заголовок каждого пакета ──────────────────────────────────────────
    #pragma pack(push, 1)

    struct Header
    {
        PacketType type;
        uint16_t   payloadSize;  // размер данных после заголовка
    };

    // C→S: первое сообщение клиента
    struct C_Hello
    {
        Header   header  = { PacketType::C_Hello, sizeof(C_Hello) - sizeof(Header) };
        uint16_t version = VERSION;
        char     name[32]{};    // ник игрока
    };

    // S→C: сервер принял игрока
    struct S_Welcome
    {
        Header   header   = { PacketType::S_Welcome, sizeof(S_Welcome) - sizeof(Header) };
        uint32_t playerID = 0;
        float    spawnX   = 0.0f;
        float    spawnY   = 2.0f;
        float    spawnZ   = 0.0f;
    };

    // S→C: сервер отклонил
    struct S_Reject
    {
        Header header = { PacketType::S_Reject, sizeof(S_Reject) - sizeof(Header) };
        char   reason[64]{};
    };

    // C→S: ввод игрока — ТОЛЬКО кнопки, не позиция!
    // Сервер сам считает позицию и отдаёт авторитетную.
    struct C_InputState
    {
        Header   header    = { PacketType::C_InputState, sizeof(C_InputState) - sizeof(Header) };
        uint32_t tick      = 0;    // номер тика клиента (для reconciliation)
        uint8_t  buttons   = 0;    // битовые флаги: bit0=forward, 1=back, 2=left, 3=right, 4=jump, 5=crouch, 6=run
        float    yaw       = 0.0f;
        float    pitch     = 0.0f;
    };

    // Флаги кнопок для C_InputState.buttons
    namespace InputBits {
        static constexpr uint8_t FORWARD  = 1 << 0;
        static constexpr uint8_t BACKWARD = 1 << 1;
        static constexpr uint8_t LEFT     = 1 << 2;
        static constexpr uint8_t RIGHT    = 1 << 3;
        static constexpr uint8_t JUMP     = 1 << 4;
        static constexpr uint8_t CROUCH   = 1 << 5;
        static constexpr uint8_t RUN      = 1 << 6;
    }

    // S→C: позиции всех игроков (авторитетное состояние)
    struct PlayerSnapshot
    {
        uint32_t playerID;
        float    x, y, z;
        float    yaw;
        uint8_t  flags;      // onGround, isCrouching, etc.
    };

    struct S_WorldState
    {
        Header   header      = { PacketType::S_WorldState, 0 }; // payloadSize считается при отправке
        uint32_t serverTick  = 0;
        uint8_t  playerCount = 0;
        // За этим заголовком идёт playerCount * sizeof(PlayerSnapshot) байт
    };

    #pragma pack(pop)
}
