#pragma once
#include "isaac_cs_t0/raw_apply_runtime.hpp"
namespace isaac_cs_t0::coop_active::sampler {
struct BindingSnapshot {
    std::array<input::PrimitiveBinding, input::kMaximumLogicalActionCount> bindings{};
    std::size_t action_count = 0;
};

bool TakeBindings(const std::uint8_t* bytes, BindingSnapshot& output) noexcept
{
    const auto* begin = *reinterpret_cast<const std::uint8_t* const*>(bytes + 0x10);
    const auto* end = *reinterpret_cast<const std::uint8_t* const*>(bytes + 0x14);
    const auto first = reinterpret_cast<std::uintptr_t>(begin);
    const auto last = reinterpret_cast<std::uintptr_t>(end);
    if (!begin || !end || last < first)
        return false;
    const std::size_t size = static_cast<std::size_t>(last - first);
    if (size < input::kGameplayActionCount * sizeof(input::PrimitiveBinding) ||
        size % sizeof(input::PrimitiveBinding) != 0 ||
        size > output.bindings.size() * sizeof(input::PrimitiveBinding))
        return false;
    output.action_count = size / sizeof(input::PrimitiveBinding);
    std::memcpy(output.bindings.data(), begin,
                input::kGameplayActionCount * sizeof(input::PrimitiveBinding));
    return true;
}

bool SameBindings(const BindingSnapshot& left,
                  const BindingSnapshot& right) noexcept
{
    return raw_apply::detail::StableBindingsEqual(
        left.bindings, left.action_count, right.bindings, right.action_count);
}

bool ReadKeyboardProjection(
    std::uint8_t* image_base, void* device, std::array<float, input::kGameplayActionCount>& values,
    std::uint16_t& valid) noexcept
{
    __try
    {
        const auto* bytes = static_cast<const std::uint8_t*>(device);
        BindingSnapshot first{};
        BindingSnapshot second{};
        std::array<std::uint8_t, raw_apply::detail::kKeyboardPrimitiveCount> state1{};
        std::array<std::uint8_t, raw_apply::detail::kKeyboardPrimitiveCount> state2{};
        if (!TakeBindings(bytes, first))
            return false;
        const bool active1 = (bytes[0x08] & 0x02u) != 0;
        for (std::uint32_t action = 0; action < input::kGameplayActionCount; ++action)
            for (std::uint32_t primitive : first.bindings[action])
                if (primitive != input::kUnboundPrimitive && primitive < state1.size())
                    state1[primitive] = image_base[0x878C10 + primitive];
        MemoryBarrier();
        if (!TakeBindings(bytes, second))
            return false;
        const bool active2 = (bytes[0x08] & 0x02u) != 0;
        for (std::uint32_t action = 0; action < input::kGameplayActionCount; ++action)
            for (std::uint32_t primitive : second.bindings[action])
                if (primitive != input::kUnboundPrimitive && primitive < state2.size())
                    state2[primitive] = image_base[0x878C10 + primitive];
        if (!SameBindings(first, second) ||
            !raw_apply::detail::StableKeyboardStateEqual(active1, state1,
                                                         active2, state2))
            return false;
        return raw_apply::detail::ProjectKeyboard(
                   second.bindings.data(), second.action_count, active2,
                   state2, values, valid) ==
               raw_apply::detail::ProjectionStatus::Ok;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool ReadGamepadProjection(
    [[maybe_unused]] std::uint8_t* image_base, void* device, std::array<float, input::kGameplayActionCount>& values,
    std::uint16_t& valid) noexcept
{
    __try
    {
        const auto* bytes = static_cast<const std::uint8_t*>(device);
        BindingSnapshot bindings1{};
        BindingSnapshot bindings2{};
        raw_apply::detail::GamepadConcreteState state1{};
        raw_apply::detail::GamepadConcreteState state2{};
        if (!TakeBindings(bytes, bindings1))
            return false;
        state1.flags = *reinterpret_cast<const std::uint16_t*>(bytes + 0x08);
        state1.button_low = *reinterpret_cast<const std::uint32_t*>(bytes + 0x20);
        state1.button_high = *reinterpret_cast<const std::uint32_t*>(bytes + 0x24);
        std::memcpy(state1.directional_axes.data(), bytes + 0x40,
                    sizeof(state1.directional_axes));
        state1.deadzone = *reinterpret_cast<const float*>(bytes + 0xD0);
        std::memcpy(state1.mappings.data(), bytes + 0x388, sizeof(state1.mappings));
        MemoryBarrier();
        if (!TakeBindings(bytes, bindings2))
            return false;
        state2.flags = *reinterpret_cast<const std::uint16_t*>(bytes + 0x08);
        state2.button_low = *reinterpret_cast<const std::uint32_t*>(bytes + 0x20);
        state2.button_high = *reinterpret_cast<const std::uint32_t*>(bytes + 0x24);
        std::memcpy(state2.directional_axes.data(), bytes + 0x40,
                    sizeof(state2.directional_axes));
        state2.deadzone = *reinterpret_cast<const float*>(bytes + 0xD0);
        std::memcpy(state2.mappings.data(), bytes + 0x388, sizeof(state2.mappings));
        if (!SameBindings(bindings1, bindings2) ||
            !raw_apply::detail::StableGamepadStateEqual(state1, state2))
            return false;
        return raw_apply::detail::ProjectGamepad(
                   bindings2.bindings.data(), bindings2.action_count, state2,
                   values, valid) == raw_apply::detail::ProjectionStatus::Ok;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}
}

