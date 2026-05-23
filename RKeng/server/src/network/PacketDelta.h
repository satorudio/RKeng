#pragma once
// PacketDelta.h — AntiLAGv1, слой 2: дельта-компрессия снапшотов.
//
// Идея: вместо полного PlayerSnapshot (20 байт × N игроков) шлём только
// изменившиеся поля. Для каждого поля — 1-битный флаг "изменилось?",
// потом только изменившиеся значения.
//
// Порог для float: если |new - old| < EPSILON — считаем не изменилось.
// Итог: при стоячем игроке пакет ~2 байта вместо 20.
// При движении — ~10-14 байт (меняется x, z, yaw).
//
// Использование:
//   // Сервер:
//   DeltaEncoder enc;
//   enc.Reset();
//   uint8_t buf[MAX_DELTA_PACKET]; size_t sz;
//   enc.Encode(prev_snapshot, new_snapshot, buf, sz);
//   PacketSend::BroadcastAll(host, buf, sz, CH_UNRELIABLE);
//
//   // Клиент:
//   DeltaDecoder dec;
//   dec.Decode(buf, sz, prev_snapshot, out_snapshot);

#include <cstdint>
#include <cstring>
#include <cmath>

namespace RKeng::Protocol
{
    static constexpr float  DELTA_EPSILON   = 0.001f;
    // Макс байт на одного игрока: playerID(4)+mask(1)+x(4)+y(4)+z(4)+yaw(4)+flags(1) = 22.
    // +1 байт size-префикс на игрока в ServerWorldTick.
    // 64 игрока * 23 + 8 байт заголовка = 1480 — влезает в MTU.
    static constexpr size_t MAX_PLAYERS      = 64;
    static constexpr size_t MAX_DELTA_SINGLE = 22;
    static constexpr size_t MAX_DELTA_PACKET = MAX_PLAYERS * (MAX_DELTA_SINGLE + 1) + 8;

    // Битовые флаги изменившихся полей PlayerSnapshot
    namespace SnapFields {
        static constexpr uint8_t X     = 1 << 0;
        static constexpr uint8_t Y     = 1 << 1;
        static constexpr uint8_t Z     = 1 << 2;
        static constexpr uint8_t YAW   = 1 << 3;
        static constexpr uint8_t FLAGS = 1 << 4;
        // bits 5-7 зарезервированы
    }

    // ── Encode: записывает дельта-снапшот в buf, возвращает размер ──────────
    // prev может быть нулевым (первый снапшот) — тогда шлём всё целиком.
    inline size_t EncodeDeltaSnapshot(
        const PlayerSnapshot* prev,
        const PlayerSnapshot& next,
        uint8_t* buf)
    {
        uint8_t* p = buf;

        // playerID всегда (4 байта)
        memcpy(p, &next.playerID, 4); p += 4;

        // Маска изменившихся полей (1 байт)
        uint8_t mask = 0;
        if (!prev || std::fabs(next.x     - prev->x)     > DELTA_EPSILON) mask |= SnapFields::X;
        if (!prev || std::fabs(next.y     - prev->y)     > DELTA_EPSILON) mask |= SnapFields::Y;
        if (!prev || std::fabs(next.z     - prev->z)     > DELTA_EPSILON) mask |= SnapFields::Z;
        if (!prev || std::fabs(next.yaw   - prev->yaw)   > DELTA_EPSILON) mask |= SnapFields::YAW;
        if (!prev || next.flags != prev->flags)                            mask |= SnapFields::FLAGS;

        *p++ = mask;

        // Только изменившиеся поля
        if (mask & SnapFields::X)     { memcpy(p, &next.x,     4); p += 4; }
        if (mask & SnapFields::Y)     { memcpy(p, &next.y,     4); p += 4; }
        if (mask & SnapFields::Z)     { memcpy(p, &next.z,     4); p += 4; }
        if (mask & SnapFields::YAW)   { memcpy(p, &next.yaw,   4); p += 4; }
        if (mask & SnapFields::FLAGS) { *p++ = next.flags; }

        return static_cast<size_t>(p - buf);
    }

    // ── Decode: применяет дельта-снапшот к prev, пишет в out ────────────────
    // prev — последнее известное состояние (обновляется in-place через out).
    inline bool DecodeDeltaSnapshot(
        const uint8_t* buf, size_t size,
        const PlayerSnapshot& prev,
        PlayerSnapshot& out)
    {
        if (size < 5) return false; // минимум: playerID(4) + mask(1)
        const uint8_t* p = buf;

        out = prev; // начинаем с предыдущего состояния
        memcpy(&out.playerID, p, 4); p += 4;
        uint8_t mask = *p++;

        size_t needed = 5;
        if (mask & SnapFields::X)     needed += 4;
        if (mask & SnapFields::Y)     needed += 4;
        if (mask & SnapFields::Z)     needed += 4;
        if (mask & SnapFields::YAW)   needed += 4;
        if (mask & SnapFields::FLAGS) needed += 1;
        if (size < needed) return false;

        if (mask & SnapFields::X)     { memcpy(&out.x,     p, 4); p += 4; }
        if (mask & SnapFields::Y)     { memcpy(&out.y,     p, 4); p += 4; }
        if (mask & SnapFields::Z)     { memcpy(&out.z,     p, 4); p += 4; }
        if (mask & SnapFields::YAW)   { memcpy(&out.yaw,   p, 4); p += 4; }
        if (mask & SnapFields::FLAGS) { out.flags = *p++; }

        return true;
    }

    // ── Утилита: считает экономию (для дебага / бенчмарка из роадмапа) ──────
    inline float DeltaSavingsPercent(size_t deltaSize)
    {
        return 100.0f * (1.0f - static_cast<float>(deltaSize) / sizeof(PlayerSnapshot));
    }
}
