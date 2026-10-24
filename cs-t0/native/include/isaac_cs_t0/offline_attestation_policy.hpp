#pragma once

#include <cstdint>

namespace isaac_cs_t0::offline_attestation {

enum class ObservationPolicy : std::uint32_t {
    ActiveRun = 0,
    LoadingMenuInactive = 1,
};

inline constexpr std::uint64_t kActiveRunHookMask = 0x3Fu;
inline constexpr std::uint64_t kLoadingMenuHookMask = 0x7Fu;
inline constexpr std::uint32_t kActiveRunSlotCount = 6;
inline constexpr std::uint32_t kLoadingMenuSlotCount = 7;

struct RunStateSample {
    bool game_present = false;
    bool readable = false;
    std::uint16_t raw = 0;
};

[[nodiscard]] constexpr bool ValidObservationPolicy(
    std::uint32_t raw) noexcept
{
    return raw == static_cast<std::uint32_t>(ObservationPolicy::ActiveRun) ||
           raw == static_cast<std::uint32_t>(
                      ObservationPolicy::LoadingMenuInactive);
}

[[nodiscard]] constexpr bool RunStateActive(const RunStateSample& sample) noexcept
{
    return sample.game_present && sample.readable && (sample.raw & 0x00FFu) != 0;
}

[[nodiscard]] constexpr bool RunStateAllowed(
    ObservationPolicy policy, const RunStateSample& sample) noexcept
{
    if (!sample.game_present || !sample.readable)
        return false;
    return policy == ObservationPolicy::LoadingMenuInactive
               ? !RunStateActive(sample)
               : RunStateActive(sample);
}

[[nodiscard]] constexpr bool IsLoadingMenuInactive(
    ObservationPolicy policy, const RunStateSample& sample) noexcept
{
    return policy == ObservationPolicy::LoadingMenuInactive &&
           RunStateAllowed(policy, sample);
}

[[nodiscard]] constexpr std::uint64_t ExpectedHookMask(
    ObservationPolicy policy) noexcept
{
    return policy == ObservationPolicy::LoadingMenuInactive
               ? kLoadingMenuHookMask
               : kActiveRunHookMask;
}

[[nodiscard]] constexpr std::uint32_t ExpectedSlotCount(
    ObservationPolicy policy) noexcept
{
    return policy == ObservationPolicy::LoadingMenuInactive
               ? kLoadingMenuSlotCount
               : kActiveRunSlotCount;
}

[[nodiscard]] constexpr bool RelocatedImageAddressMatches(
    std::uintptr_t image_base, std::uint32_t operand,
    std::uint32_t target_rva) noexcept
{
    return operand == static_cast<std::uint32_t>(image_base + target_rva);
}

struct ManagerNegativeSample {
    std::uintptr_t manager = 0;
    std::uintptr_t network_backend = 0;
    std::uintptr_t roster_first = 0;
    std::uintptr_t roster_last = 0;
    std::uintptr_t roster_capacity = 0;
    std::uint32_t online_frame = 0;
    bool readable = false;
};

// The frozen OriginalOnline manager identity is not manager-slot presence. It requires the
// manager frame path together with an active network backend. The manager object is routinely
// allocated in a local run, so either passive policy proves the same conservative negative:
// backend absent, frame zero and a canonical empty roster throughout the callback window.
[[nodiscard]] constexpr bool ManagerActivitySignal(
    const ManagerNegativeSample& sample) noexcept
{
    return !sample.readable || sample.network_backend != 0 || sample.online_frame != 0;
}

[[nodiscard]] constexpr bool RosterSignal(const ManagerNegativeSample& sample) noexcept
{
    if (!sample.readable)
        return true;
    return sample.roster_first != 0 || sample.roster_last != 0 ||
           sample.roster_capacity != 0;
}

[[nodiscard]] constexpr bool SameManagerIdentity(const ManagerNegativeSample& first,
                                                 const ManagerNegativeSample& current) noexcept
{
    return first.readable && current.readable && first.manager == current.manager;
}

[[nodiscard]] constexpr bool ObservationWindowOpen(bool steam_observer_armed,
                                                   bool unloading) noexcept
{
    return steam_observer_armed && !unloading;
}

} // namespace isaac_cs_t0::offline_attestation
