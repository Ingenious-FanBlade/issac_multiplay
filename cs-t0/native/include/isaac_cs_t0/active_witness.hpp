#pragma once

#include "isaac_cs_t0/active_ipc.hpp"
#include "isaac_cs_t0/sha256.hpp"

#include <cstdint>

namespace isaac_cs_t0::active {

struct PlayerIdentity {
    std::uint32_t ordinal = UINT32_MAX;
    std::uint32_t device_key = UINT32_MAX;
    std::uint64_t owner_generation = 0;
};

enum class WitnessStatus : std::uint32_t {
    Ok = 0,
    NoPlayerScope = 1,
    NestedPlayer = 2,
    OwnerAba = 3,
    CrossFamily = 4,
    ReadFault = 5,
};

bool EnterPlayerScope(void* player, const PlayerIdentity& identity) noexcept;
bool LeavePlayerScope(void* player, const PlayerIdentity& identity) noexcept;
bool EnterConsumerScope(void* player, active_abi::Consumer consumer,
                        std::uint64_t owner_generation) noexcept;
bool LeaveConsumerScope(void* player, active_abi::Consumer consumer,
                        std::uint64_t owner_generation) noexcept;
WitnessStatus CaptureWitness(void* player, active_abi::Consumer consumer,
                             mode::Sha256Digest& digest) noexcept;
void ClearWitnessTls() noexcept;

[[nodiscard]] active_abi::Consumer CurrentConsumer() noexcept;
[[nodiscard]] PlayerIdentity CurrentPlayerIdentity() noexcept;

} // namespace isaac_cs_t0::active
