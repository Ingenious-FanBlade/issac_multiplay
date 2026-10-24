#pragma once

#include "isaac_cs_t0/mode_ipc.hpp"

#include <array>
#include <cstdint>

namespace isaac_cs_t0::mode {

inline constexpr std::uint32_t kOfflineCallbackThreshold = 60;

enum class RequestedMode : std::uint32_t {
    Offline = 1,
    OriginalOnline = 2,
    CustomAuthorityServer = 3,
    CustomAuthorityClient = 4,
};

struct SignalObservation {
    bool lua_generation_bound = false;
    bool simulation_callback = false;
    bool manager_identity = false;
    bool stable_roster = false;
    bool channel_zero_activity = false;
    bool nonzero_channel = false;
    bool wrong_thread = false;
    bool manager_changed = false;
    bool frame_regressed = false;
    bool lua_closed = false;
    bool returned_to_menu = false;
};

enum class TransitionKind {
    None,
    Frozen,
    Retired,
};

struct ModeTransition {
    TransitionKind kind = TransitionKind::None;
    NetworkMode mode = NetworkMode::Unresolved;
};

class ModeLatch final {
public:
    explicit ModeLatch(RequestedMode requested) noexcept;

    bool BeginGeneration(std::uint64_t token) noexcept;
    ModeTransition Observe(const SignalObservation& signal) noexcept;
    void Retire() noexcept;

    [[nodiscard]] NetworkMode State() const noexcept { return state_; }
    [[nodiscard]] std::uint64_t Generation() const noexcept { return generation_; }
    [[nodiscard]] std::uint64_t Token() const noexcept { return token_; }
    [[nodiscard]] std::uint32_t OnlineSignalCount() const noexcept;

private:
    [[nodiscard]] bool WasRetired(std::uint64_t token) const noexcept;
    void RememberRetired(std::uint64_t token) noexcept;

    RequestedMode requested_;
    NetworkMode state_ = NetworkMode::Unresolved;
    std::uint64_t generation_ = 0;
    std::uint64_t token_ = 0;
    std::array<std::uint64_t, 16> retired_tokens_{};
    std::size_t retired_count_ = 0;
    bool lua_generation_bound_ = false;
    std::uint32_t simulation_callbacks_ = 0;
    std::uint32_t manager_observations_ = 0;
    std::uint32_t roster_observations_ = 0;
    std::uint32_t channel_zero_calls_ = 0;
};

} // namespace isaac_cs_t0::mode
