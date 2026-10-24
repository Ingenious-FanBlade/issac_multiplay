#pragma once

#include "isaac_cs_t0/active_ipc.hpp"
#include "isaac_cs_t0/input_core.hpp"

#include <cstdint>

namespace isaac_cs_t0::active {

struct QueryEligibility {
    bool main_thread = false;
    bool steady_standard = false;
    bool unique_player = false;
    bool unique_device = false;
    bool natural_consumer = false;
    std::uint64_t apply_generation = 0;
    std::uint64_t owner_generation = 0;
    active_abi::Consumer consumer = active_abi::Consumer::None;
};

enum class OverrideDisposition : std::uint32_t {
    Passthrough = 0,
    SuppressNeutral = 1,
    Snapshot = 2,
    TerminalPassthrough = 3,
};

struct QueryOverride {
    OverrideDisposition disposition = OverrideDisposition::Passthrough;
    float scalar = 0.0F;
    bool boolean = false;
};

active_abi::Consumer ConsumerForAction(std::uint32_t action) noexcept;
float ActionMagnitude(const input::InputStateDraftV1& state,
                      std::uint32_t action) noexcept;
bool ActionTriggered(const input::InputStateDraftV1& previous,
                     const input::InputStateDraftV1& current,
                     std::uint32_t action) noexcept;

class SyntheticAdapter final {
public:
    void SetPendingTarget(std::uint64_t target_tick) noexcept;
    bool BeginAuthority(std::uint64_t candidate_tick, std::uint64_t apply_generation,
                        std::uint64_t owner_generation,
                        const input::InputStateDraftV1* resolved_state) noexcept;
    bool EndAuthority(bool original_returned_normally) noexcept;
    QueryOverride Evaluate(std::uint32_t action, active_abi::QueryKind query,
                           const QueryEligibility& eligibility) const noexcept;
    void ClearGeneration() noexcept;
    void Retire() noexcept;

    [[nodiscard]] bool ScopeActive() const noexcept { return authority_scope_; }
    [[nodiscard]] bool SnapshotActive() const noexcept { return snapshot_active_; }
    [[nodiscard]] const input::InputStateDraftV1& Snapshot() const noexcept {
        return snapshot_;
    }
    [[nodiscard]] const input::InputStateDraftV1& Previous() const noexcept {
        return previous_;
    }

private:
    input::InputStateDraftV1 snapshot_{};
    input::InputStateDraftV1 previous_{};
    std::uint64_t authority_tick_ = 0;
    std::uint64_t apply_generation_ = 0;
    std::uint64_t owner_generation_ = 0;
    std::uint64_t pending_target_tick_ = 0;
    bool authority_scope_ = false;
    bool snapshot_active_ = false;
    bool retired_ = false;
};

} // namespace isaac_cs_t0::active
