#include "isaac_cs_t0/active_witness.hpp"

#include <array>
#include <cstring>

namespace isaac_cs_t0::active {
namespace {

struct WitnessTls {
    void* player = nullptr;
    PlayerIdentity identity{};
    active_abi::Consumer consumer = active_abi::Consumer::None;
    bool consumer_active = false;
};

thread_local WitnessTls g_tls{};

template <std::size_t Size>
void Append(std::array<std::uint8_t, 32>& payload, std::size_t& cursor,
            const void* source) noexcept
{
    static_assert(Size <= 32);
    std::memcpy(payload.data() + cursor, source, Size);
    cursor += Size;
}

} // namespace

bool EnterPlayerScope(void* player, const PlayerIdentity& identity) noexcept
{
    if (!player || identity.ordinal == UINT32_MAX ||
        identity.device_key == UINT32_MAX || identity.owner_generation == 0 ||
        g_tls.player)
        return false;
    g_tls.player = player;
    g_tls.identity = identity;
    return true;
}

bool LeavePlayerScope(void* player, const PlayerIdentity& identity) noexcept
{
    const bool matches = g_tls.player == player &&
                         g_tls.identity.ordinal == identity.ordinal &&
                         g_tls.identity.device_key == identity.device_key &&
                         g_tls.identity.owner_generation == identity.owner_generation &&
                         !g_tls.consumer_active;
    g_tls = {};
    return matches;
}

bool EnterConsumerScope(void* player, active_abi::Consumer consumer,
                        std::uint64_t owner_generation) noexcept
{
    if (!player || g_tls.player != player || g_tls.consumer_active ||
        consumer == active_abi::Consumer::None ||
        owner_generation == 0 ||
        g_tls.identity.owner_generation != owner_generation)
        return false;
    g_tls.consumer = consumer;
    g_tls.consumer_active = true;
    return true;
}

bool LeaveConsumerScope(void* player, active_abi::Consumer consumer,
                        std::uint64_t owner_generation) noexcept
{
    const bool matches = g_tls.player == player && g_tls.consumer_active &&
                         g_tls.consumer == consumer &&
                         g_tls.identity.owner_generation == owner_generation;
    g_tls.consumer = active_abi::Consumer::None;
    g_tls.consumer_active = false;
    return matches;
}

WitnessStatus CaptureWitness(void* player, active_abi::Consumer consumer,
                             mode::Sha256Digest& digest) noexcept
{
    digest = {};
    if (!player || g_tls.player != player || !g_tls.consumer_active)
        return WitnessStatus::NoPlayerScope;
    if (g_tls.consumer != consumer)
        return WitnessStatus::CrossFamily;
    std::array<std::uint8_t, 32> payload{};
    std::size_t cursor = 0;
    const std::uint32_t version = 1;
    const std::uint32_t family = static_cast<std::uint32_t>(consumer);
    Append<sizeof(version)>(payload, cursor, &version);
    Append<sizeof(family)>(payload, cursor, &family);
    __try
    {
        const auto* bytes = static_cast<const std::uint8_t*>(player);
        switch (consumer)
        {
        case active_abi::Consumer::Movement:
            Append<4>(payload, cursor, bytes + 0x1624);
            Append<4>(payload, cursor, bytes + 0x0360);
            Append<4>(payload, cursor, bytes + 0x0364);
            break;
        case active_abi::Consumer::Bomb:
        {
            const std::uint32_t intent =
                *reinterpret_cast<const std::uint32_t*>(bytes + 0x1378) & 1u;
            Append<4>(payload, cursor, &intent);
            break;
        }
        case active_abi::Consumer::Shooting:
            Append<4>(payload, cursor, bytes + 0x162C);
            Append<4>(payload, cursor, bytes + 0x1630);
            Append<4>(payload, cursor, bytes + 0x1634);
            Append<1>(payload, cursor, bytes + 0x1638);
            break;
        case active_abi::Consumer::Drop:
            Append<4>(payload, cursor, bytes + 0x17C0);
            break;
        case active_abi::Consumer::Item:
        case active_abi::Consumer::PillCard:
            // The frozen passive contract deliberately has no fabricated universal
            // world field for these families.  Their tagged digest proves the exact
            // before/after consumer bracket without claiming a world mutation field.
            break;
        default:
            return WitnessStatus::CrossFamily;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return WitnessStatus::ReadFault;
    }
    digest = mode::Sha256(payload.data(), cursor);
    return WitnessStatus::Ok;
}

void ClearWitnessTls() noexcept
{
    g_tls = {};
}

active_abi::Consumer CurrentConsumer() noexcept
{
    return g_tls.consumer_active ? g_tls.consumer : active_abi::Consumer::None;
}

PlayerIdentity CurrentPlayerIdentity() noexcept
{
    return g_tls.player ? g_tls.identity : PlayerIdentity{};
}

} // namespace isaac_cs_t0::active
