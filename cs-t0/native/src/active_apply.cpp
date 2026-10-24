#include "isaac_cs_t0/active_apply.hpp"

#include <algorithm>

namespace isaac_cs_t0::active {
namespace {

float ComponentMagnitude(std::int16_t value, bool negative) noexcept
{
    const std::int32_t promoted = value;
    const std::int32_t directional =
        negative ? (std::max)(-promoted, 0) : (std::max)(promoted, 0);
    return static_cast<float>(directional) /
           static_cast<float>(input::kAxisMaximum);
}

} // namespace

active_abi::Consumer ConsumerForAction(std::uint32_t action) noexcept
{
    if (action <= 3)
        return active_abi::Consumer::Movement;
    if (action <= 7)
        return active_abi::Consumer::Shooting;
    switch (action)
    {
    case 8:
        return active_abi::Consumer::Bomb;
    case 9:
        return active_abi::Consumer::Item;
    case 10:
        return active_abi::Consumer::PillCard;
    case 11:
        return active_abi::Consumer::Drop;
    default:
        return active_abi::Consumer::None;
    }
}

float ActionMagnitude(const input::InputStateDraftV1& state,
                      std::uint32_t action) noexcept
{
    switch (action)
    {
    case 0:
        return ComponentMagnitude(state.move_x, true);
    case 1:
        return ComponentMagnitude(state.move_x, false);
    case 2:
        return ComponentMagnitude(state.move_y, true);
    case 3:
        return ComponentMagnitude(state.move_y, false);
    case 4:
        return ComponentMagnitude(state.aim_x, true);
    case 5:
        return ComponentMagnitude(state.aim_x, false);
    case 6:
        return ComponentMagnitude(state.aim_y, true);
    case 7:
        return ComponentMagnitude(state.aim_y, false);
    case 8:
    case 9:
    case 10:
    case 11:
        return (state.edge_flags & (1u << (action - 8))) != 0 ? 1.0F : 0.0F;
    default:
        return 0.0F;
    }
}

bool ActionTriggered(const input::InputStateDraftV1& previous,
                     const input::InputStateDraftV1& current,
                     std::uint32_t action) noexcept
{
    if (action < 8)
        return ActionMagnitude(previous, action) == 0.0F &&
               ActionMagnitude(current, action) != 0.0F;
    if (action < input::kGameplayActionCount)
        return (current.edge_flags & (1u << (action - 8))) != 0;
    return false;
}

void SyntheticAdapter::SetPendingTarget(std::uint64_t target_tick) noexcept
{
    pending_target_tick_ = target_tick;
}

bool SyntheticAdapter::BeginAuthority(
    std::uint64_t candidate_tick, std::uint64_t apply_generation,
    std::uint64_t owner_generation,
    const input::InputStateDraftV1* resolved_state) noexcept
{
    if (retired_ || authority_scope_ || candidate_tick == 0 ||
        apply_generation == 0 || owner_generation == 0)
        return false;
    authority_scope_ = true;
    authority_tick_ = candidate_tick;
    apply_generation_ = apply_generation;
    owner_generation_ = owner_generation;
    snapshot_active_ = resolved_state != nullptr;
    if (resolved_state)
    {
        snapshot_ = *resolved_state;
        pending_target_tick_ = 0;
    }
    return true;
}

bool SyntheticAdapter::EndAuthority(bool original_returned_normally) noexcept
{
    if (!authority_scope_)
        return false;
    if (original_returned_normally)
    {
        if (snapshot_active_)
            previous_ = snapshot_;
        else if (pending_target_tick_ > authority_tick_)
            previous_ = {};
    }
    authority_scope_ = false;
    snapshot_active_ = false;
    snapshot_ = {};
    authority_tick_ = 0;
    apply_generation_ = 0;
    owner_generation_ = 0;
    return original_returned_normally;
}

QueryOverride SyntheticAdapter::Evaluate(
    std::uint32_t action, active_abi::QueryKind query,
    const QueryEligibility& eligibility) const noexcept
{
    QueryOverride result{};
    if (retired_)
    {
        result.disposition = OverrideDisposition::TerminalPassthrough;
        return result;
    }
    if (action >= input::kGameplayActionCount || !authority_scope_)
        return result;
    // Logical queries unrelated to a natural Player consumer remain transparent.
    if (!eligibility.natural_consumer ||
        eligibility.consumer == active_abi::Consumer::None)
        return result;
    if (!eligibility.main_thread || !eligibility.steady_standard ||
        !eligibility.unique_player || !eligibility.unique_device ||
        eligibility.apply_generation != apply_generation_ ||
        eligibility.owner_generation != owner_generation_ ||
        eligibility.consumer != ConsumerForAction(action))
    {
        result.disposition = OverrideDisposition::TerminalPassthrough;
        return result;
    }
    if (!snapshot_active_)
    {
        if (pending_target_tick_ > authority_tick_)
            result.disposition = OverrideDisposition::SuppressNeutral;
        return result;
    }
    result.disposition = OverrideDisposition::Snapshot;
    result.scalar = ActionMagnitude(snapshot_, action);
    switch (query)
    {
    case active_abi::QueryKind::Pressed:
        result.boolean = result.scalar != 0.0F;
        break;
    case active_abi::QueryKind::Triggered:
        result.boolean = ActionTriggered(previous_, snapshot_, action);
        break;
    case active_abi::QueryKind::Scalar:
        result.boolean = result.scalar != 0.0F;
        break;
    default:
        result.disposition = OverrideDisposition::TerminalPassthrough;
        break;
    }
    return result;
}

void SyntheticAdapter::ClearGeneration() noexcept
{
    snapshot_ = {};
    previous_ = {};
    authority_tick_ = 0;
    apply_generation_ = 0;
    owner_generation_ = 0;
    pending_target_tick_ = 0;
    authority_scope_ = false;
    snapshot_active_ = false;
}

void SyntheticAdapter::Retire() noexcept
{
    ClearGeneration();
    retired_ = true;
}

} // namespace isaac_cs_t0::active
