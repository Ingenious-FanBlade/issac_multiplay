#include "isaac_cs_t0/mode_latch.hpp"

#include <limits>

namespace isaac_cs_t0::mode {
namespace {

std::uint32_t SaturatingIncrement(std::uint32_t value) noexcept
{
    return value == (std::numeric_limits<std::uint32_t>::max)() ? value : value + 1;
}

NetworkMode FrozenMode(RequestedMode requested) noexcept
{
    return static_cast<NetworkMode>(requested);
}

bool LiveAllowed(RequestedMode requested) noexcept
{
    return requested == RequestedMode::Offline || requested == RequestedMode::OriginalOnline;
}

} // namespace

ModeLatch::ModeLatch(RequestedMode requested) noexcept : requested_(requested) {}

bool ModeLatch::BeginGeneration(std::uint64_t token) noexcept
{
    if (!token || WasRetired(token) ||
        (state_ != NetworkMode::Unresolved && state_ != NetworkMode::Retired) ||
        (state_ == NetworkMode::Unresolved && token_ != 0) ||
        generation_ == (std::numeric_limits<std::uint64_t>::max)())
        return false;
    ++generation_;
    token_ = token;
    state_ = NetworkMode::Unresolved;
    lua_generation_bound_ = false;
    simulation_callbacks_ = 0;
    manager_observations_ = 0;
    roster_observations_ = 0;
    channel_zero_calls_ = 0;
    if (!LiveAllowed(requested_))
    {
        Retire();
        return false;
    }
    return true;
}

ModeTransition ModeLatch::Observe(const SignalObservation& signal) noexcept
{
    if (state_ == NetworkMode::Retired || token_ == 0)
        return {};
    if (signal.wrong_thread || signal.manager_changed || signal.frame_regressed ||
        signal.lua_closed || signal.returned_to_menu || signal.nonzero_channel)
    {
        Retire();
        return {TransitionKind::Retired, NetworkMode::Retired};
    }
    lua_generation_bound_ = lua_generation_bound_ || signal.lua_generation_bound;
    if (signal.simulation_callback)
        simulation_callbacks_ = SaturatingIncrement(simulation_callbacks_);
    if (signal.manager_identity)
        manager_observations_ = SaturatingIncrement(manager_observations_);
    if (signal.stable_roster)
        roster_observations_ = SaturatingIncrement(roster_observations_);
    if (signal.channel_zero_activity)
        channel_zero_calls_ = SaturatingIncrement(channel_zero_calls_);

    if (state_ != NetworkMode::Unresolved)
    {
        if (state_ != FrozenMode(requested_))
        {
            Retire();
            return {TransitionKind::Retired, NetworkMode::Retired};
        }
        return {};
    }

    const std::uint32_t online_signals = OnlineSignalCount();
    const bool offline_ready = lua_generation_bound_ &&
                               simulation_callbacks_ >= kOfflineCallbackThreshold &&
                               manager_observations_ == 0 && roster_observations_ == 0 &&
                               channel_zero_calls_ == 0;
    if (requested_ == RequestedMode::OriginalOnline && online_signals >= 2)
    {
        state_ = NetworkMode::OriginalOnline;
        return {TransitionKind::Frozen, state_};
    }
    if (requested_ == RequestedMode::Offline && online_signals >= 2)
    {
        Retire();
        return {TransitionKind::Retired, NetworkMode::Retired};
    }
    if (requested_ == RequestedMode::Offline && offline_ready)
    {
        state_ = NetworkMode::Offline;
        return {TransitionKind::Frozen, state_};
    }
    if (requested_ == RequestedMode::OriginalOnline && offline_ready)
    {
        Retire();
        return {TransitionKind::Retired, NetworkMode::Retired};
    }
    return {};
}

void ModeLatch::Retire() noexcept
{
    if (token_)
        RememberRetired(token_);
    token_ = 0;
    state_ = NetworkMode::Retired;
}

std::uint32_t ModeLatch::OnlineSignalCount() const noexcept
{
    return static_cast<std::uint32_t>(manager_observations_ >= 2) +
           static_cast<std::uint32_t>(roster_observations_ >= 2) +
           static_cast<std::uint32_t>(channel_zero_calls_ != 0);
}

bool ModeLatch::WasRetired(std::uint64_t token) const noexcept
{
    for (std::size_t index = 0; index < retired_count_; ++index)
    {
        if (retired_tokens_[index] == token)
            return true;
    }
    return false;
}

void ModeLatch::RememberRetired(std::uint64_t token) noexcept
{
    if (WasRetired(token))
        return;
    if (retired_count_ < retired_tokens_.size())
    {
        retired_tokens_[retired_count_++] = token;
        return;
    }
    for (std::size_t index = 1; index < retired_tokens_.size(); ++index)
        retired_tokens_[index - 1] = retired_tokens_[index];
    retired_tokens_.back() = token;
}

} // namespace isaac_cs_t0::mode
