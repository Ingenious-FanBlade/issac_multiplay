#pragma once

#include <cstdint>

namespace isaac_cs_t0::mode {

inline constexpr std::uint32_t kLifecycleUnavailable = UINT32_MAX;

template <typename Function>
[[nodiscard]] inline std::uint32_t InvokeLifecycleOnce(Function original,
                                                       std::uint32_t argument) noexcept
{
    return original(argument);
}

[[nodiscard]] constexpr bool IsRunActive(std::uint32_t value) noexcept
{
    return value != kLifecycleUnavailable && (value & 0xFFu) != 0;
}

[[nodiscard]] constexpr bool IsStartObservation(std::uint32_t before,
                                                std::uint32_t after) noexcept
{
    return IsRunActive(before) && IsRunActive(after);
}

[[nodiscard]] constexpr bool IsTeardownObservation(std::uint32_t before,
                                                   std::uint32_t after,
                                                   bool same_game) noexcept
{
    return same_game && IsRunActive(before) && after == 0;
}

} // namespace isaac_cs_t0::mode
