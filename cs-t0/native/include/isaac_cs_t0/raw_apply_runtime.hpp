#pragma once

#include "isaac_cs_t0/input_core.hpp"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace isaac_cs_t0::raw_apply {

namespace detail {

inline constexpr std::size_t kKeyboardPrimitiveCount = 0x15D;
inline constexpr std::size_t kGamepadDirectionalAxisCount = 32;
inline constexpr std::size_t kGamepadMappingCount = 29;
inline constexpr std::uint32_t kGamepadAxisPrimitiveBase = 0xABAB;
inline constexpr std::uint32_t kGamepadSpecialPrimitiveFirst = 0xCDCC;
inline constexpr std::uint32_t kGamepadSpecialPrimitiveSecond = 0xCDCD;
inline constexpr std::uint32_t kPhysicalPollReturnRva = 0x0061F30C;
inline constexpr std::uint32_t kInputQueryPhase = 3;

inline constexpr bool PhysicalProjectionContextEligible(std::uint32_t return_rva,
                                                        std::uint32_t phase) noexcept
{
    return return_rva == kPhysicalPollReturnRva && phase == kInputQueryPhase;
}

inline constexpr bool ProjectionPayloadScopeValid(std::uint32_t action,
                                                   std::uint32_t primitive) noexcept
{
    return action < input::kGameplayActionCount && primitive == UINT32_MAX;
}

// v1's device+0xC0/+0xC4 hypothesis was disproved by canary and static evidence.  Keep an
// explicit compile-time assertion point so tests can prove that code cannot silently revive it.
inline constexpr bool LegacyActionCacheProjectionEnabled() noexcept
{
    return false;
}

enum class ProjectionStatus : std::uint32_t {
    Ok = 0,
    InvalidLayout,
    AmbiguousBinding,
    UnsupportedSpecial,
    NonFinite,
};

struct GamepadMapping final {
    std::uint32_t logical_primitive = input::kUnboundPrimitive;
    std::uint32_t physical_code = input::kUnboundPrimitive;
};

struct GamepadConcreteState final {
    std::uint16_t flags = 0;
    std::uint32_t button_low = 0;
    std::uint32_t button_high = 0;
    std::array<float, kGamepadDirectionalAxisCount> directional_axes{};
    float deadzone = 0.0F;
    std::array<GamepadMapping, kGamepadMappingCount> mappings{};
};

struct ProjectionHistory final {
    bool valid = false;
    std::uint32_t device_key = UINT32_MAX;
    std::uint16_t valid_actions = 0;
    std::array<float, input::kGameplayActionCount> values{};
};

struct ProjectionEdgeMasks final {
    std::uint16_t rising = 0;
    std::uint16_t falling = 0;
};

inline bool StableBindingsEqual(
    const std::array<input::PrimitiveBinding, input::kMaximumLogicalActionCount>& first,
    std::size_t first_count,
    const std::array<input::PrimitiveBinding, input::kMaximumLogicalActionCount>& second,
    std::size_t second_count) noexcept
{
    return first_count == second_count &&
           first_count <= input::kMaximumLogicalActionCount &&
           std::memcmp(first.data(), second.data(),
                       (std::min)(first_count, input::kGameplayActionCount) *
                           sizeof(input::PrimitiveBinding)) == 0;
}

inline bool StableKeyboardStateEqual(
    bool first_active,
    const std::array<std::uint8_t, kKeyboardPrimitiveCount>& first,
    bool second_active,
    const std::array<std::uint8_t, kKeyboardPrimitiveCount>& second) noexcept
{
    return first_active == second_active && first == second;
}

inline bool StableGamepadStateEqual(const GamepadConcreteState& first,
                                    const GamepadConcreteState& second) noexcept
{
    return first.flags == second.flags && first.button_low == second.button_low &&
           first.button_high == second.button_high &&
           std::memcmp(first.directional_axes.data(), second.directional_axes.data(),
                       sizeof(first.directional_axes)) == 0 &&
           std::memcmp(&first.deadzone, &second.deadzone, sizeof(first.deadzone)) == 0 &&
           std::memcmp(first.mappings.data(), second.mappings.data(),
                       sizeof(first.mappings)) == 0;
}

inline bool ProjectionBindingLayoutValid(const input::PrimitiveBinding* bindings,
                                         std::size_t action_count) noexcept
{
    return bindings && action_count >= input::kGameplayActionCount &&
           action_count <= input::kMaximumLogicalActionCount;
}

inline bool ProjectionBindingOwnedOnlyByGameplayAction(
    const input::PrimitiveBinding* bindings, std::size_t action_count,
    std::uint32_t action, std::uint32_t primitive) noexcept
{
    if (!ProjectionBindingLayoutValid(bindings, action_count) ||
        action >= input::kGameplayActionCount || primitive == input::kUnboundPrimitive)
        return false;
    bool owned_by_action = false;
    for (std::uint32_t owner = 0; owner < input::kGameplayActionCount; ++owner)
    {
        if (std::find(bindings[owner].begin(), bindings[owner].end(), primitive) ==
            bindings[owner].end())
            continue;
        if (owner != action)
            return false;
        owned_by_action = true;
    }
    return owned_by_action;
}

inline ProjectionStatus ProjectKeyboard(
    const input::PrimitiveBinding* bindings, std::size_t action_count, bool active,
    const std::array<std::uint8_t, kKeyboardPrimitiveCount>& current,
    std::array<float, input::kGameplayActionCount>& output,
    std::uint16_t& valid_actions) noexcept
{
    output.fill(0.0F);
    valid_actions = 0;
    if (!ProjectionBindingLayoutValid(bindings, action_count))
        return ProjectionStatus::InvalidLayout;
    for (std::uint32_t action = 0; action < input::kGameplayActionCount; ++action)
    {
        bool valid = true;
        for (std::uint32_t primitive : bindings[action])
        {
            if (primitive == input::kUnboundPrimitive)
                continue;
            if (!ProjectionBindingOwnedOnlyByGameplayAction(bindings, action_count, action,
                                                            primitive) ||
                primitive >= current.size())
            {
                valid = false;
                break;
            }
            if (active && current[primitive] != 0)
                output[action] = 1.0F;
        }
        if (valid)
            valid_actions |= static_cast<std::uint16_t>(1u << action);
    }
    return ProjectionStatus::Ok;
}

inline ProjectionStatus ResolveGamepadPhysicalCode(
    const GamepadConcreteState& state, std::uint32_t primitive,
    std::uint32_t& physical_code, bool& mapped) noexcept
{
    mapped = (state.flags & 0x20u) == 0;
    physical_code = primitive;
    if (!mapped)
        return ProjectionStatus::Ok;
    std::size_t matches = 0;
    for (const GamepadMapping& entry : state.mappings)
    {
        if (entry.logical_primitive == primitive)
        {
            physical_code = entry.physical_code;
            ++matches;
        }
    }
    if (matches > 1)
        return ProjectionStatus::InvalidLayout;
    if (matches == 0)
        physical_code = input::kUnboundPrimitive;
    return ProjectionStatus::Ok;
}

inline ProjectionStatus GamepadPrimitiveValue(const GamepadConcreteState& state,
                                              std::uint32_t primitive,
                                              float& output) noexcept
{
    output = 0.0F;
    // The original primitive selects the structured +0x64 path after both ordinary
    // classifiers fail.  v2 does not let a mutable mapping disguise this unsupported class.
    if (primitive == kGamepadSpecialPrimitiveFirst ||
        primitive == kGamepadSpecialPrimitiveSecond)
        return ProjectionStatus::UnsupportedSpecial;
    std::uint32_t physical_code = primitive;
    bool mapped = false;
    const ProjectionStatus resolved =
        ResolveGamepadPhysicalCode(state, primitive, physical_code, mapped);
    if (resolved != ProjectionStatus::Ok)
        return resolved;

    if (physical_code < kGamepadAxisPrimitiveBase)
    {
        if (physical_code < 64)
        {
            const std::uint32_t word = physical_code < 32 ? state.button_low
                                                          : state.button_high;
            output = (word & (1u << (physical_code & 31u))) != 0 ? 1.0F : 0.0F;
        }
        return ProjectionStatus::Ok;
    }

    const std::uint32_t axis_index = physical_code - kGamepadAxisPrimitiveBase;
    const bool axis_category = mapped
                                   ? physical_code != input::kUnboundPrimitive
                                   : axis_index < kGamepadDirectionalAxisCount;
    if (axis_category)
    {
        if (axis_index >= kGamepadDirectionalAxisCount ||
            !std::isfinite(state.deadzone) || state.deadzone < 0.0F ||
            state.deadzone >= 1.0F)
            return ProjectionStatus::InvalidLayout;
        const float value = state.directional_axes[axis_index];
        // Gamepad poll RVA 0x622940 has already split each physical axis into two
        // non-negative directional magnitudes before storing device+0x40[0..31].
        if (!std::isfinite(value))
            return ProjectionStatus::NonFinite;
        if (value < 0.0F)
            return ProjectionStatus::InvalidLayout;
        if (value > state.deadzone)
            output = (value - state.deadzone) / (1.0F - state.deadzone);
        if (!std::isfinite(output))
            return ProjectionStatus::NonFinite;
        return ProjectionStatus::Ok;
    }

    return ProjectionStatus::Ok;
}

inline ProjectionStatus ProjectGamepad(
    const input::PrimitiveBinding* bindings, std::size_t action_count,
    const GamepadConcreteState& state,
    std::array<float, input::kGameplayActionCount>& output,
    std::uint16_t& valid_actions) noexcept
{
    output.fill(0.0F);
    valid_actions = 0;
    if (!ProjectionBindingLayoutValid(bindings, action_count))
        return ProjectionStatus::InvalidLayout;
    for (std::uint32_t action = 0; action < input::kGameplayActionCount; ++action)
    {
        bool valid = true;
        for (std::uint32_t primitive : bindings[action])
        {
            if (primitive == input::kUnboundPrimitive)
                continue;
            if (!ProjectionBindingOwnedOnlyByGameplayAction(bindings, action_count, action,
                                                            primitive))
            {
                valid = false;
                break;
            }
            float value = 0.0F;
            const ProjectionStatus value_status =
                GamepadPrimitiveValue(state, primitive, value);
            if (value_status != ProjectionStatus::Ok)
            {
                valid = false;
                break;
            }
            if ((state.flags & 0x2u) != 0)
                output[action] = (std::max)(output[action], value);
        }
        if (valid)
            valid_actions |= static_cast<std::uint16_t>(1u << action);
    }
    return ProjectionStatus::Ok;
}

inline ProjectionEdgeMasks UpdateProjectionHistory(
    ProjectionHistory& history, std::uint32_t device_key,
    const std::array<float, input::kGameplayActionCount>& current,
    std::uint16_t valid_actions) noexcept
{
    ProjectionEdgeMasks edges{};
    if (history.valid && history.device_key == device_key)
    {
        for (std::uint32_t action = 0; action < input::kGameplayActionCount; ++action)
        {
            const std::uint16_t action_bit = static_cast<std::uint16_t>(1u << action);
            if ((history.valid_actions & action_bit) == 0 ||
                (valid_actions & action_bit) == 0)
                continue;
            const bool was_active = history.values[action] != 0.0F;
            const bool is_active = current[action] != 0.0F;
            if (!was_active && is_active)
                edges.rising |= action_bit;
            else if (was_active && !is_active)
                edges.falling |= action_bit;
        }
    }
    history.valid = valid_actions != 0;
    history.device_key = device_key;
    history.valid_actions = valid_actions;
    history.values = current;
    return edges;
}

} // namespace detail

void SetModule(HMODULE module) noexcept;
void ProcessDetach() noexcept;
bool Active() noexcept;
DWORD Initialize(const wchar_t* mapping_name) noexcept;
DWORD Shutdown() noexcept;

} // namespace isaac_cs_t0::raw_apply
