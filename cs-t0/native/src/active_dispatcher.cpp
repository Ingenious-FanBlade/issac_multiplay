#include "isaac_cs_t0/active_dispatcher.hpp"

#include <algorithm>
#include <limits>

namespace isaac_cs_t0::active {

namespace {

bool SamePersistentState(const input::InputStateDraftV1& left,
                         const input::InputStateDraftV1& right) noexcept
{
    return left.version == right.version &&
           left.encoded_length == right.encoded_length &&
           left.held_flags == right.held_flags && left.move_x == right.move_x &&
           left.move_y == right.move_y && left.aim_x == right.aim_x &&
           left.aim_y == right.aim_y && left.reserved == right.reserved;
}

bool IsOneShotRejectScenario(active_abi::Scenario scenario) noexcept
{
    return scenario == active_abi::Scenario::QueueFull ||
           scenario == active_abi::Scenario::Expired ||
           scenario == active_abi::Scenario::TooFar ||
           scenario == active_abi::Scenario::Stale;
}

} // namespace

bool DispatchBatch::Push(const DispatchTrace& trace) noexcept
{
    if (count == traces.size())
    {
        error = active_abi::ErrorCode::ProtocolViolation;
        client_retired = true;
        return false;
    }
    traces[count++] = trace;
    return true;
}

active_abi::RejectReason PublicRejectReason(input::RejectReason reason) noexcept
{
    switch (reason)
    {
    case input::RejectReason::QueueFull:
        return active_abi::RejectReason::QueueFull;
    case input::RejectReason::TargetExpired:
        return active_abi::RejectReason::Expired;
    case input::RejectReason::TargetTooFar:
        return active_abi::RejectReason::TooFar;
    case input::RejectReason::StaleSequence:
        return active_abi::RejectReason::Stale;
    case input::RejectReason::SequenceConflict:
        return active_abi::RejectReason::IdentityConflict;
    default:
        return active_abi::RejectReason::None;
    }
}

ActiveDispatcher::ActiveDispatcher(const std::array<std::uint8_t, 16>& session_id,
                                   std::uint64_t client_id,
                                   active_abi::Scenario scenario) noexcept
    : session_id_(session_id),
      client_id_(client_id),
      scenario_(scenario),
      queue_(session_id, client_id, input::kBaseValidEdgeMask)
{
}

void ActiveDispatcher::SeedQueueFull(std::uint64_t current_tick) noexcept
{
    input::InputCmdDraftV1 seed{};
    seed.session_id = session_id_;
    seed.client_id = client_id_;
    seed.target_tick = current_tick + input::kMaximumFutureTicks;
    for (std::uint64_t sequence = 1; sequence <= input::kInputQueueCapacity; ++sequence)
    {
        seed.sequence = sequence;
        const input::AcceptResult result =
            queue_.Accept(seed, current_tick, input::TickPhase::DrainBeforeResolve);
        if (result.decision != input::AcceptDecision::Accepted)
        {
            retired_ = true;
            return;
        }
    }
    next_command_sequence_ = input::kInputQueueCapacity + 1;
    last_accepted_sequence_ = input::kInputQueueCapacity;
    pending_target_tick_ = seed.target_tick;
}

void ActiveDispatcher::SeedStale(std::uint64_t current_tick) noexcept
{
    input::InputCmdDraftV1 seed{};
    seed.session_id = session_id_;
    seed.client_id = client_id_;
    seed.sequence = 2;
    seed.target_tick = current_tick + input::kInputLeadTicks;
    if (queue_.Accept(seed, current_tick, input::TickPhase::DrainBeforeResolve).decision !=
        input::AcceptDecision::Accepted)
        retired_ = true;
    next_command_sequence_ = 1;
    last_accepted_sequence_ = seed.sequence;
    pending_target_tick_ = seed.target_tick;
}

void ActiveDispatcher::ConfigureNegativeScenario(
    std::uint64_t current_tick, input::InputCmdDraftV1& command) noexcept
{
    if (scenario_seeded_)
        return;
    switch (scenario_)
    {
    case active_abi::Scenario::QueueFull:
        SeedQueueFull(current_tick);
        command.sequence = next_command_sequence_++;
        break;
    case active_abi::Scenario::Expired:
        command.target_tick = current_tick == 0 ? 0 : current_tick - 1;
        break;
    case active_abi::Scenario::TooFar:
        command.target_tick = current_tick + input::kMaximumFutureTicks + 1;
        break;
    case active_abi::Scenario::Stale:
        SeedStale(current_tick);
        next_command_sequence_ = 3;
        break;
    default:
        break;
    }
    scenario_seeded_ = true;
}

void ActiveDispatcher::RecordAccept(const input::InputCmdDraftV1& command,
                                    active_abi::SourceDevice source,
                                    std::uint32_t attempt,
                                    const input::AcceptResult& accepted,
                                    DispatchBatch& batch) noexcept
{
    DispatchTrace trace{};
    trace.command = command;
    trace.source = source;
    trace.attempt = attempt;
    trace.phase = active_abi::Phase::DrainBeforeResolve;
    trace.queue_depth = static_cast<std::uint32_t>(accepted.depth_after);
    switch (accepted.decision)
    {
    case input::AcceptDecision::Accepted:
        trace.kind = active_abi::EventKind::Accept;
        trace.decision = active_abi::Decision::Accepted;
        pending_target_tick_ =
            pending_target_tick_ == 0
                ? command.target_tick
                : (std::min)(pending_target_tick_, command.target_tick);
        pending_command_ = command;
        pending_source_ = source;
        last_accepted_sequence_ = (std::max)(last_accepted_sequence_, command.sequence);
        break;
    case input::AcceptDecision::Duplicate:
        trace.kind = active_abi::EventKind::Duplicate;
        trace.decision = active_abi::Decision::Duplicate;
        break;
    case input::AcceptDecision::FatalConflict:
        trace.kind = active_abi::EventKind::Reject;
        trace.decision = active_abi::Decision::Rejected;
        trace.reject = active_abi::RejectReason::IdentityConflict;
        trace.previous_command_sequence = command.sequence;
        retired_ = true;
        batch.client_retired = true;
        break;
    case input::AcceptDecision::Rejected:
        trace.kind = active_abi::EventKind::Reject;
        trace.decision = active_abi::Decision::Rejected;
        trace.reject = PublicRejectReason(accepted.reason);
        if (trace.reject == active_abi::RejectReason::Stale)
            trace.previous_command_sequence = last_accepted_sequence_;
        if (trace.reject == active_abi::RejectReason::None)
        {
            trace.kind = active_abi::EventKind::Failure;
            trace.phase = active_abi::Phase::Outside;
            trace.decision = active_abi::Decision::None;
            trace.terminal_protocol_failure = true;
            retired_ = true;
            batch.client_retired = true;
            batch.error = active_abi::ErrorCode::ProtocolViolation;
        }
        else
        {
            // Scenario-construction seeds are never world-applicable commands.  Once
            // the public reject has been established, discard the entire synthetic
            // setup so a later authority tick cannot resolve it.
            queue_.Clear();
            pending_target_tick_ = 0;
            pending_command_ = {};
            pending_source_ = active_abi::SourceDevice::None;
            if (IsOneShotRejectScenario(scenario_))
                negative_scenario_completed_ = true;
        }
        break;
    }
    batch.Push(trace);
}

bool ActiveDispatcher::SubmitEncoded(const input::InputCmdDraftV1& command,
                                     active_abi::SourceDevice source,
                                     std::uint32_t attempt,
                                     DispatchBatch& batch) noexcept
{
    input::EncodedInputCmdV1 encoded{};
    if (input::EncodeInputCommand(command, input::kBaseValidEdgeMask, encoded) !=
        input::CodecError::None)
    {
        batch.error = active_abi::ErrorCode::ProtocolViolation;
        batch.client_retired = true;
        retired_ = true;
        return false;
    }
    DispatchTrace send{};
    send.kind = active_abi::EventKind::Send;
    send.phase = active_abi::Phase::PhysicalSample;
    send.command = command;
    send.source = source;
    send.attempt = attempt;
    if (!batch.Push(send))
        return false;
    if (transport_.Send(encoded.data(), encoded.size()) != input::TransportSendResult::Sent)
    {
        batch.error = active_abi::ErrorCode::ProtocolViolation;
        batch.client_retired = true;
        retired_ = true;
        return false;
    }
    std::array<input::EncodedInputCmdV1, 2> drained{};
    const std::size_t count = transport_.Drain(drained.data(), drained.size(), 1);
    if (count != 1)
    {
        batch.error = active_abi::ErrorCode::ProtocolViolation;
        batch.client_retired = true;
        retired_ = true;
        return false;
    }
    input::InputCmdDraftV1 decoded{};
    if (input::DecodeInputCommand(drained[0].data(), drained[0].size(),
                                  input::kBaseValidEdgeMask, decoded) !=
        input::CodecError::None)
    {
        batch.error = active_abi::ErrorCode::ProtocolViolation;
        batch.client_retired = true;
        retired_ = true;
        return false;
    }
    DispatchTrace receive = send;
    receive.kind = active_abi::EventKind::Receive;
    receive.phase = active_abi::Phase::DrainBeforeResolve;
    receive.command = decoded;
    if (!batch.Push(receive))
        return false;
    const input::AcceptResult accepted =
        queue_.Accept(decoded, current_tick_, input::TickPhase::DrainBeforeResolve);
    RecordAccept(decoded, source, attempt, accepted, batch);
    return !batch.client_retired || accepted.decision == input::AcceptDecision::FatalConflict;
}

DispatchBatch ActiveDispatcher::SubmitPhysical(const input::RawInputSample& raw,
                                               active_abi::SourceDevice source,
                                               std::uint64_t observed_completed_tick) noexcept
{
    DispatchBatch batch{};
    current_tick_ = observed_completed_tick;
    if (retired_)
    {
        batch.client_retired = true;
        batch.error = active_abi::ErrorCode::ProtocolViolation;
        return batch;
    }
    if (negative_scenario_completed_)
        return batch;
    // There is no tick earlier than zero.  Delay the one-shot Expired fixture
    // until a real predecessor exists instead of accepting target zero and
    // permanently consuming the scenario arm.
    if (scenario_ == active_abi::Scenario::Expired && !scenario_seeded_ &&
        observed_completed_tick == 0)
        return batch;
    input::InputStateDraftV1 canonical{};
    if (input::CanonicalizeRaw(raw, input::kBaseValidEdgeMask, canonical) !=
        input::CodecError::None)
    {
        batch.client_retired = true;
        batch.error = active_abi::ErrorCode::ProtocolViolation;
        retired_ = true;
        return batch;
    }
    // V5 can observe the selected device more than once before the same
    // authority tick completes.  A second identical level-only observation in
    // that exact completed-tick/source scope is redundant.  Do not deduplicate
    // across completed ticks: persistent held/axis state must still produce one
    // synthetic snapshot per authority tick.  Edges and state transitions are
    // always submitted so the frozen same-target merge semantics remain intact.
    if (canonical.edge_flags == 0 && last_same_tick_state_valid_ &&
        last_same_tick_observed_tick_ == observed_completed_tick &&
        last_same_tick_source_ == source &&
        SamePersistentState(last_same_tick_state_, canonical))
        return batch;
    input::InputCmdDraftV1 command{};
    command.session_id = session_id_;
    command.client_id = client_id_;
    command.sequence = next_command_sequence_++;
    command.state = canonical;
    if (planner_.Plan(observed_completed_tick, canonical, command.target_tick) !=
        input::EdgePlanResult::Planned)
    {
        batch.client_retired = true;
        batch.error = active_abi::ErrorCode::AuthorityOverflow;
        retired_ = true;
        return batch;
    }
    ConfigureNegativeScenario(observed_completed_tick, command);
    if (retired_)
    {
        batch.client_retired = true;
        batch.error = active_abi::ErrorCode::ProtocolViolation;
        return batch;
    }

    DispatchTrace canonical_trace{};
    canonical_trace.kind = active_abi::EventKind::Canonical;
    canonical_trace.phase = active_abi::Phase::PhysicalSample;
    canonical_trace.command = command;
    canonical_trace.source = source;
    canonical_trace.attempt = 0;
    batch.Push(canonical_trace);
    SubmitEncoded(command, source, 0, batch);

    const bool primary_accepted =
        std::any_of(batch.traces.begin(), batch.traces.begin() + batch.count,
                    [](const DispatchTrace& trace) {
                        return trace.attempt == 0 &&
                               trace.kind == active_abi::EventKind::Accept &&
                               trace.decision == active_abi::Decision::Accepted;
                    });
    if (primary_accepted)
    {
        last_same_tick_state_ = canonical;
        last_same_tick_state_.edge_flags = 0;
        last_same_tick_observed_tick_ = observed_completed_tick;
        last_same_tick_source_ = source;
        last_same_tick_state_valid_ = true;
    }

    if (scenario_ == active_abi::Scenario::Duplicate && !batch.client_retired)
        SubmitEncoded(command, source, 1, batch);
    else if (scenario_ == active_abi::Scenario::Conflict && !batch.client_retired)
    {
        input::InputCmdDraftV1 conflict = command;
        conflict.state.move_x = conflict.state.move_x == 0 ? 1024 : 0;
        const std::size_t conflict_begin = batch.count;
        SubmitEncoded(conflict, source, 1, batch);
        // Keep the primary command payload immutable and carry the bytes that
        // actually conflicted in their dedicated field on every retry stage.
        for (std::size_t index = conflict_begin; index < batch.count; ++index)
        {
            batch.traces[index].command = command;
            batch.traces[index].conflicting_state = conflict.state;
            batch.traces[index].has_conflicting_state = true;
        }
    }
    return batch;
}

AuthorityResolution ActiveDispatcher::Resolve(std::uint64_t candidate_tick,
                                              DispatchBatch& trace) noexcept
{
    AuthorityResolution output{};
    output.target_tick = candidate_tick;
    if (retired_)
        return output;
    const input::ResolveResult result = queue_.Resolve(candidate_tick);
    output.resolved = result.resolved;
    output.has_input = result.has_input;
    if (result.has_input)
    {
        output.state = result.effective.state;
        output.provenance = result.effective.provenance_sequences;
        output.provenance_count = result.effective.provenance_count;
        output.source = pending_source_;
    }
    DispatchTrace event{};
    event.kind = active_abi::EventKind::Resolve;
    event.phase = active_abi::Phase::Resolve;
    event.command.session_id = session_id_;
    event.command.client_id = client_id_;
    event.command.target_tick = candidate_tick;
    event.decision = active_abi::Decision::None;
    event.queue_depth = static_cast<std::uint32_t>(queue_.Size());
    if (result.effective.provenance_count)
        event.command.sequence =
            result.effective.provenance_sequences[result.effective.provenance_count - 1];
    if (result.has_input)
        event.command.state = result.effective.state;
    trace.Push(event);
    if (pending_target_tick_ != 0 && candidate_tick >= pending_target_tick_)
    {
        pending_target_tick_ = 0;
        pending_command_ = {};
        pending_source_ = active_abi::SourceDevice::None;
    }
    return output;
}

bool ActiveDispatcher::PendingCommand(input::InputCmdDraftV1& command,
                                      active_abi::SourceDevice& source) const noexcept
{
    if (pending_target_tick_ == 0 || pending_command_.sequence == 0)
        return false;
    command = pending_command_;
    source = pending_source_;
    return true;
}

void ActiveDispatcher::ClearForGeneration() noexcept
{
    transport_.Clear();
    queue_.Clear();
    planner_.Reset();
    pending_target_tick_ = 0;
    pending_command_ = {};
    pending_source_ = active_abi::SourceDevice::None;
    last_same_tick_state_ = {};
    last_same_tick_observed_tick_ = 0;
    last_same_tick_source_ = active_abi::SourceDevice::None;
    last_same_tick_state_valid_ = false;
    last_accepted_sequence_ = 0;
}

void ActiveDispatcher::Retire() noexcept
{
    retired_ = true;
    transport_.Clear();
    queue_.Retire();
    planner_.Reset();
    pending_target_tick_ = 0;
    pending_command_ = {};
    pending_source_ = active_abi::SourceDevice::None;
    last_same_tick_state_ = {};
    last_same_tick_observed_tick_ = 0;
    last_same_tick_source_ = active_abi::SourceDevice::None;
    last_same_tick_state_valid_ = false;
    last_accepted_sequence_ = 0;
}

} // namespace isaac_cs_t0::active
