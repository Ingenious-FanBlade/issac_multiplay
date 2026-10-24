#pragma once

#include "isaac_cs_t0/active_ipc.hpp"
#include "isaac_cs_t0/input_core.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace isaac_cs_t0::active {

struct DispatchTrace {
    active_abi::EventKind kind = active_abi::EventKind::Failure;
    active_abi::Phase phase = active_abi::Phase::Outside;
    active_abi::Decision decision = active_abi::Decision::None;
    active_abi::RejectReason reject = active_abi::RejectReason::None;
    active_abi::SourceDevice source = active_abi::SourceDevice::None;
    input::InputCmdDraftV1 command{};
    input::InputStateDraftV1 conflicting_state{};
    std::uint32_t attempt = 0;
    std::uint32_t queue_depth = 0;
    std::uint64_t previous_command_sequence = 0;
    bool has_conflicting_state = false;
    bool terminal_protocol_failure = false;
};

struct DispatchBatch {
    std::array<DispatchTrace, 24> traces{};
    std::size_t count = 0;
    bool client_retired = false;
    active_abi::ErrorCode error = active_abi::ErrorCode::None;

    bool Push(const DispatchTrace& trace) noexcept;
};

struct AuthorityResolution {
    bool resolved = false;
    bool has_input = false;
    input::InputStateDraftV1 state{};
    std::array<std::uint64_t, input::kInputQueueCapacity> provenance{};
    std::size_t provenance_count = 0;
    std::uint64_t target_tick = 0;
    active_abi::SourceDevice source = active_abi::SourceDevice::None;
};

class ActiveDispatcher final {
public:
    ActiveDispatcher(const std::array<std::uint8_t, 16>& session_id,
                     std::uint64_t client_id,
                     active_abi::Scenario scenario) noexcept;

    DispatchBatch SubmitPhysical(const input::RawInputSample& raw,
                                 active_abi::SourceDevice source,
                                 std::uint64_t observed_completed_tick) noexcept;
    AuthorityResolution Resolve(std::uint64_t candidate_tick,
                                DispatchBatch& trace) noexcept;
    void ClearForGeneration() noexcept;
    void Retire() noexcept;

    [[nodiscard]] bool Retired() const noexcept { return retired_ || queue_.Retired(); }
    [[nodiscard]] std::uint64_t PendingTargetTick() const noexcept {
        return pending_target_tick_;
    }
    [[nodiscard]] std::uint64_t LastCommandSequence() const noexcept {
        return next_command_sequence_ == 0 ? 0 : next_command_sequence_ - 1;
    }
    [[nodiscard]] bool PendingCommand(input::InputCmdDraftV1& command,
                                      active_abi::SourceDevice& source) const noexcept;

private:
    void ConfigureNegativeScenario(std::uint64_t current_tick,
                                   input::InputCmdDraftV1& command) noexcept;
    bool SubmitEncoded(const input::InputCmdDraftV1& command,
                       active_abi::SourceDevice source, std::uint32_t attempt,
                       DispatchBatch& batch) noexcept;
    void RecordAccept(const input::InputCmdDraftV1& command,
                      active_abi::SourceDevice source, std::uint32_t attempt,
                      const input::AcceptResult& accepted,
                      DispatchBatch& batch) noexcept;
    void SeedQueueFull(std::uint64_t current_tick) noexcept;
    void SeedStale(std::uint64_t current_tick) noexcept;

    std::array<std::uint8_t, 16> session_id_{};
    std::uint64_t client_id_ = 0;
    active_abi::Scenario scenario_ = active_abi::Scenario::Keyboard;
    input::LoopbackTransport transport_{};
    input::InputQueue queue_;
    input::EdgeTargetPlanner planner_{input::kBaseValidEdgeMask};
    std::uint64_t next_command_sequence_ = 1;
    std::uint64_t last_accepted_sequence_ = 0;
    std::uint64_t current_tick_ = 0;
    std::uint64_t pending_target_tick_ = 0;
    input::InputCmdDraftV1 pending_command_{};
    active_abi::SourceDevice pending_source_ = active_abi::SourceDevice::None;
    input::InputStateDraftV1 last_same_tick_state_{};
    std::uint64_t last_same_tick_observed_tick_ = 0;
    active_abi::SourceDevice last_same_tick_source_ =
        active_abi::SourceDevice::None;
    bool last_same_tick_state_valid_ = false;
    bool scenario_seeded_ = false;
    bool negative_scenario_completed_ = false;
    bool retired_ = false;
};

active_abi::RejectReason PublicRejectReason(input::RejectReason reason) noexcept;

} // namespace isaac_cs_t0::active
