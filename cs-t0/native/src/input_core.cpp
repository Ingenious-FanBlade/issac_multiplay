#include "isaac_cs_t0/input_core.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <utility>

namespace isaac_cs_t0::input {
namespace {

std::uint16_t ReadU16(const std::uint8_t* bytes) noexcept
{
    return static_cast<std::uint16_t>(bytes[0]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[1]) << 8);
}

std::uint32_t ReadU32(const std::uint8_t* bytes) noexcept
{
    return static_cast<std::uint32_t>(bytes[0]) |
           (static_cast<std::uint32_t>(bytes[1]) << 8) |
           (static_cast<std::uint32_t>(bytes[2]) << 16) |
           (static_cast<std::uint32_t>(bytes[3]) << 24);
}

std::uint64_t ReadU64(const std::uint8_t* bytes) noexcept
{
    std::uint64_t value = 0;
    for (std::size_t index = 0; index < sizeof(value); ++index)
        value |= static_cast<std::uint64_t>(bytes[index]) << (index * 8);
    return value;
}

void WriteU16(std::uint8_t* bytes, std::uint16_t value) noexcept
{
    bytes[0] = static_cast<std::uint8_t>(value);
    bytes[1] = static_cast<std::uint8_t>(value >> 8);
}

void WriteU32(std::uint8_t* bytes, std::uint32_t value) noexcept
{
    for (std::size_t index = 0; index < sizeof(value); ++index)
        bytes[index] = static_cast<std::uint8_t>(value >> (index * 8));
}

void WriteU64(std::uint8_t* bytes, std::uint64_t value) noexcept
{
    for (std::size_t index = 0; index < sizeof(value); ++index)
        bytes[index] = static_cast<std::uint8_t>(value >> (index * 8));
}

std::uint64_t IntegerSquareRootFloor(std::uint64_t value) noexcept
{
    if (value < 2)
        return value;
    std::uint64_t low = 1;
    std::uint64_t high = (std::min)(value, static_cast<std::uint64_t>(UINT32_MAX));
    std::uint64_t answer = 1;
    while (low <= high)
    {
        const std::uint64_t middle = low + (high - low) / 2;
        if (middle <= value / middle)
        {
            answer = middle;
            low = middle + 1;
        }
        else
            high = middle - 1;
    }
    return answer;
}

std::uint64_t IntegerSquareRootCeil(std::uint64_t value) noexcept
{
    const std::uint64_t floor = IntegerSquareRootFloor(value);
    return floor + static_cast<std::uint64_t>(floor * floor != value);
}

CodecError QuantizeRaw(float value, std::int32_t& output) noexcept
{
    if (!std::isfinite(value))
        return CodecError::NonFiniteRaw;
    const float clamped = (std::max)(-1.0F, (std::min)(value, 1.0F));
    output = static_cast<std::int32_t>(
        std::trunc(clamped * static_cast<float>(kAxisMaximum)));
    return CodecError::None;
}

void NormalizeVector(std::int32_t input_x, std::int32_t input_y, std::int16_t& output_x,
                     std::int16_t& output_y) noexcept
{
    const std::int64_t x = input_x;
    const std::int64_t y = input_y;
    const std::uint64_t squared = static_cast<std::uint64_t>(x * x + y * y);
    const std::uint64_t magnitude = IntegerSquareRootCeil(squared);
    if (magnitude <= static_cast<std::uint64_t>(kRadialDeadZone))
    {
        output_x = 0;
        output_y = 0;
        return;
    }
    const std::int64_t wide_magnitude = static_cast<std::int64_t>(magnitude);
    const std::int64_t capped_magnitude = (std::min)(
        wide_magnitude, static_cast<std::int64_t>(kAxisMaximum));
    const std::int64_t output_magnitude =
        (capped_magnitude - kRadialDeadZone) * kAxisMaximum /
        (kAxisMaximum - kRadialDeadZone);
    const std::int64_t normalized_x = x * output_magnitude / wide_magnitude;
    const std::int64_t normalized_y = y * output_magnitude / wide_magnitude;
    output_x = static_cast<std::int16_t>(normalized_x);
    output_y = static_cast<std::int16_t>(normalized_y);
}

bool VectorInsideUnitCircle(std::int16_t x, std::int16_t y) noexcept
{
    if (x == (std::numeric_limits<std::int16_t>::min)() ||
        y == (std::numeric_limits<std::int16_t>::min)())
        return false;
    const std::int64_t wide_x = x;
    const std::int64_t wide_y = y;
    const std::int64_t maximum = kAxisMaximum;
    return wide_x * wide_x + wide_y * wide_y <= maximum * maximum;
}

bool NonzeroSession(const std::array<std::uint8_t, 16>& session) noexcept
{
    return std::any_of(session.begin(), session.end(), [](std::uint8_t value) {
        return value != 0;
    });
}

bool SameCommand(const InputCmdDraftV1& left, const InputCmdDraftV1& right) noexcept
{
    return left.session_id == right.session_id && left.client_id == right.client_id &&
           left.sequence == right.sequence && left.target_tick == right.target_tick &&
           left.state.version == right.state.version &&
           left.state.encoded_length == right.state.encoded_length &&
           left.state.held_flags == right.state.held_flags &&
           left.state.edge_flags == right.state.edge_flags && left.state.move_x == right.state.move_x &&
           left.state.move_y == right.state.move_y && left.state.aim_x == right.state.aim_x &&
           left.state.aim_y == right.state.aim_y && left.state.reserved == right.state.reserved;
}

} // namespace

const char* InputConfigDecisionName(InputConfigDecision decision) noexcept
{
    switch(decision) {
    case InputConfigDecision::Accepted:return "Accepted";
    case InputConfigDecision::InvalidMappingVersion:return "InvalidMappingVersion";
    case InputConfigDecision::InvalidMappingSize:return "InvalidMappingSize";
    case InputConfigDecision::InvalidConfigVersion:return "InvalidConfigVersion";
    case InputConfigDecision::InvalidConfigSize:return "InvalidConfigSize";
    case InputConfigDecision::ConfigReservedNonzero:return "ConfigReservedNonzero";
    case InputConfigDecision::LeadOutOfRange:return "LeadOutOfRange";
    case InputConfigDecision::FutureWindowOutOfRange:return "FutureWindowOutOfRange";
    case InputConfigDecision::LeadExceedsFutureWindow:return "LeadExceedsFutureWindow";
    case InputConfigDecision::MaxInputAgeUnsupported:return "MaxInputAgeUnsupported";
    case InputConfigDecision::ScriptBudgetOutOfRange:return "ScriptBudgetOutOfRange";
    case InputConfigDecision::AdmissionBudgetOutOfRange:return "AdmissionBudgetOutOfRange";
    case InputConfigDecision::AdmissionBudgetBelowMinimum:return "AdmissionBudgetBelowMinimum";
    }
    return "UNKNOWN";
}
InputConfigDecision ValidateInputRuntimeConfig(const InputRuntimeConfig& config) noexcept
{
    if(config.version!=1) return InputConfigDecision::InvalidConfigVersion;
    if(config.size!=sizeof(InputRuntimeConfig)) return InputConfigDecision::InvalidConfigSize;
    if(config.reserved!=0) return InputConfigDecision::ConfigReservedNonzero;
    if(config.input_lead_ticks<kRuntimeInputLeadMinimum ||
       config.input_lead_ticks>kRuntimeInputLeadMaximum)
        return InputConfigDecision::LeadOutOfRange;
    if(config.future_window_ticks<kRuntimeFutureWindowMinimum ||
       config.future_window_ticks>kRuntimeFutureWindowMaximum)
        return InputConfigDecision::FutureWindowOutOfRange;
    if(config.input_lead_ticks>config.future_window_ticks)
        return InputConfigDecision::LeadExceedsFutureWindow;
    if(config.max_input_age_ticks!=0)
        return InputConfigDecision::MaxInputAgeUnsupported;
    if(config.script_drain_budget<kRuntimeScriptDrainMinimum ||
       config.script_drain_budget>kRuntimeScriptDrainMaximum)
        return InputConfigDecision::ScriptBudgetOutOfRange;
    const std::uint64_t minimum_admission=2ull+4ull*config.script_drain_budget;
    if(config.admission_attempt_budget>kRuntimeAdmissionAttemptMaximum)
        return InputConfigDecision::AdmissionBudgetOutOfRange;
    if(config.admission_attempt_budget<minimum_admission)
        return InputConfigDecision::AdmissionBudgetBelowMinimum;
    return InputConfigDecision::Accepted;
}
bool NextInputTarget(std::uint64_t completed_tick,const InputRuntimeConfig& config,
                     std::uint64_t& target_tick) noexcept
{
    if(config.input_lead_ticks<kRuntimeInputLeadMinimum ||
       config.input_lead_ticks>kRuntimeInputLeadMaximum ||
       completed_tick>(std::numeric_limits<std::uint64_t>::max)()-config.input_lead_ticks)
        return false;
    target_tick=completed_tick+config.input_lead_ticks;return true;
}
bool NextStartupTarget(std::uint64_t completed_tick,std::uint64_t& target_tick) noexcept
{
    if(completed_tick==(std::numeric_limits<std::uint64_t>::max)()) return false;
    target_tick=completed_tick+1;return true;
}
bool ScriptDrainAllowed(std::uint64_t drained_steps,const InputRuntimeConfig& config) noexcept
{
    return drained_steps<config.script_drain_budget;
}

PrimitiveActionClassification ClassifyPrimitiveAction(
    std::uint32_t primitive, const PrimitiveBinding* bindings,
    std::size_t action_count) noexcept
{
    if (action_count > kMaximumLogicalActionCount || (!bindings && action_count != 0))
        return {PrimitiveActionClass::InvalidLayout, UINT32_MAX};
    if (primitive == kUnboundPrimitive)
        return {};
    std::size_t matched_action = kMaximumLogicalActionCount;
    for (std::size_t action = 0; action < action_count; ++action)
    {
        if (std::find(bindings[action].begin(), bindings[action].end(), primitive) ==
            bindings[action].end())
            continue;
        if (matched_action != kMaximumLogicalActionCount)
            return {PrimitiveActionClass::AmbiguousOrOutsideGameplay, UINT32_MAX};
        matched_action = action;
    }
    if (matched_action == kMaximumLogicalActionCount)
        return {};
    if (matched_action >= kGameplayActionCount)
        return {PrimitiveActionClass::AmbiguousOrOutsideGameplay, UINT32_MAX};
    return {PrimitiveActionClass::UniqueGameplay,
            static_cast<std::uint32_t>(matched_action)};
}

CodecError CanonicalizeRaw(const RawInputSample& raw, std::uint32_t valid_edge_mask,
                           InputStateDraftV1& output) noexcept
{
    valid_edge_mask &= kBaseValidEdgeMask;
    if ((raw.held_flags & ~kValidHeldMask) != 0)
        return CodecError::UnknownHeldFlags;
    if ((raw.edge_flags & ~valid_edge_mask) != 0)
        return CodecError::UnknownEdgeFlags;
    std::int32_t move_x = 0;
    std::int32_t move_y = 0;
    std::int32_t aim_x = 0;
    std::int32_t aim_y = 0;
    for (const auto pair : {
             std::pair{raw.move_x, &move_x}, std::pair{raw.move_y, &move_y},
             std::pair{raw.aim_x, &aim_x}, std::pair{raw.aim_y, &aim_y},
         })
    {
        const CodecError error = QuantizeRaw(pair.first, *pair.second);
        if (error != CodecError::None)
            return error;
    }

    InputStateDraftV1 state{};
    state.held_flags = raw.held_flags;
    state.edge_flags = raw.edge_flags;
    NormalizeVector(move_x, move_y, state.move_x, state.move_y);
    NormalizeVector(aim_x, aim_y, state.aim_x, state.aim_y);
    const CodecError error = ValidateInputState(state, valid_edge_mask);
    if (error == CodecError::None)
        output = state;
    return error;
}

CodecError ValidateInputState(const InputStateDraftV1& state,
                              std::uint32_t valid_edge_mask) noexcept
{
    valid_edge_mask &= kBaseValidEdgeMask;
    if (state.version != kInputStateVersion)
        return CodecError::InvalidVersion;
    if (state.encoded_length != kInputStateEncodedSize)
        return CodecError::InvalidLength;
    if ((state.held_flags & ~kValidHeldMask) != 0)
        return CodecError::UnknownHeldFlags;
    if ((state.edge_flags & ~valid_edge_mask) != 0)
        return CodecError::UnknownEdgeFlags;
    if (state.reserved != 0)
        return CodecError::ReservedNonzero;
    if (state.move_x == (std::numeric_limits<std::int16_t>::min)() ||
        state.move_y == (std::numeric_limits<std::int16_t>::min)() ||
        state.aim_x == (std::numeric_limits<std::int16_t>::min)() ||
        state.aim_y == (std::numeric_limits<std::int16_t>::min)())
        return CodecError::AxisOutOfRange;
    if (!VectorInsideUnitCircle(state.move_x, state.move_y) ||
        !VectorInsideUnitCircle(state.aim_x, state.aim_y))
        return CodecError::VectorOutOfRange;
    const bool shoot = (state.held_flags & kHeldShoot) != 0;
    const bool aim = state.aim_x != 0 || state.aim_y != 0;
    if (shoot != aim)
        return CodecError::ShootAimMismatch;
    return CodecError::None;
}

CodecError EncodeInputState(const InputStateDraftV1& state, std::uint32_t valid_edge_mask,
                            EncodedInputStateV1& output) noexcept
{
    const CodecError error = ValidateInputState(state, valid_edge_mask);
    if (error != CodecError::None)
        return error;
    output.fill(0);
    WriteU16(output.data(), state.version);
    WriteU16(output.data() + 2, state.encoded_length);
    WriteU32(output.data() + 4, state.held_flags);
    WriteU32(output.data() + 8, state.edge_flags);
    WriteU16(output.data() + 12, static_cast<std::uint16_t>(state.move_x));
    WriteU16(output.data() + 14, static_cast<std::uint16_t>(state.move_y));
    WriteU16(output.data() + 16, static_cast<std::uint16_t>(state.aim_x));
    WriteU16(output.data() + 18, static_cast<std::uint16_t>(state.aim_y));
    WriteU32(output.data() + 20, state.reserved);
    return CodecError::None;
}

CodecError DecodeInputState(const void* bytes, std::size_t size, std::uint32_t valid_edge_mask,
                            InputStateDraftV1& output) noexcept
{
    if (!bytes || size != kInputStateEncodedSize)
        return CodecError::InvalidSize;
    const auto* input = static_cast<const std::uint8_t*>(bytes);
    InputStateDraftV1 state{};
    state.version = ReadU16(input);
    state.encoded_length = ReadU16(input + 2);
    state.held_flags = ReadU32(input + 4);
    state.edge_flags = ReadU32(input + 8);
    state.move_x = static_cast<std::int16_t>(ReadU16(input + 12));
    state.move_y = static_cast<std::int16_t>(ReadU16(input + 14));
    state.aim_x = static_cast<std::int16_t>(ReadU16(input + 16));
    state.aim_y = static_cast<std::int16_t>(ReadU16(input + 18));
    state.reserved = ReadU32(input + 20);
    const CodecError error = ValidateInputState(state, valid_edge_mask);
    if (error == CodecError::None)
        output = state;
    return error;
}

CodecError EncodeInputCommand(const InputCmdDraftV1& command, std::uint32_t valid_edge_mask,
                              EncodedInputCmdV1& output) noexcept
{
    EncodedInputStateV1 state{};
    const CodecError error = EncodeInputState(command.state, valid_edge_mask, state);
    if (error != CodecError::None)
        return error;
    output.fill(0);
    std::copy(command.session_id.begin(), command.session_id.end(), output.begin());
    WriteU64(output.data() + 16, command.client_id);
    WriteU64(output.data() + 24, command.sequence);
    WriteU64(output.data() + 32, command.target_tick);
    std::copy(state.begin(), state.end(), output.begin() + 40);
    return CodecError::None;
}

CodecError DecodeInputCommand(const void* bytes, std::size_t size, std::uint32_t valid_edge_mask,
                              InputCmdDraftV1& output) noexcept
{
    if (!bytes || size != kInputCommandEncodedSize)
        return CodecError::InvalidSize;
    const auto* input = static_cast<const std::uint8_t*>(bytes);
    InputCmdDraftV1 command{};
    std::copy(input, input + command.session_id.size(), command.session_id.begin());
    command.client_id = ReadU64(input + 16);
    command.sequence = ReadU64(input + 24);
    command.target_tick = ReadU64(input + 32);
    const CodecError error =
        DecodeInputState(input + 40, kInputStateEncodedSize, valid_edge_mask, command.state);
    if (error == CodecError::None)
        output = command;
    return error;
}

TransportSendResult LoopbackTransport::Send(const void* bytes, std::size_t size) noexcept
{
    if (!bytes || size != kInputCommandEncodedSize)
        return TransportSendResult::InvalidSize;
    if (count_ == messages_.size())
        return TransportSendResult::Full;
    std::memcpy(messages_[tail_].data(), bytes, size);
    tail_ = (tail_ + 1) % messages_.size();
    ++count_;
    return TransportSendResult::Sent;
}

std::size_t LoopbackTransport::Drain(EncodedInputCmdV1* output, std::size_t output_capacity,
                                     std::uint32_t maximum_messages) noexcept
{
    if (!output || output_capacity == 0 || maximum_messages == 0)
        return 0;
    const std::size_t maximum = (std::min)(
        count_, (std::min)(output_capacity, static_cast<std::size_t>(maximum_messages)));
    for (std::size_t index = 0; index < maximum; ++index)
    {
        output[index] = messages_[head_];
        head_ = (head_ + 1) % messages_.size();
    }
    count_ -= maximum;
    return maximum;
}

void LoopbackTransport::Clear() noexcept
{
    head_ = 0;
    tail_ = 0;
    count_ = 0;
}

InputQueue::InputQueue(const std::array<std::uint8_t, 16>& session_id, std::uint64_t client_id,
                       std::uint32_t valid_edge_mask) noexcept
    : session_id_(session_id), client_id_(client_id),
      valid_edge_mask_(valid_edge_mask & kBaseValidEdgeMask)
{
    if (!NonzeroSession(session_id_) || client_id_ == 0)
        retired_ = true;
}

const InputQueue::HistoryEntry* InputQueue::FindHistory(std::uint64_t sequence) const noexcept
{
    for (std::size_t index = 0; index < history_count_; ++index)
    {
        if (history_[index].occupied && history_[index].command.sequence == sequence)
            return &history_[index];
    }
    return nullptr;
}

void InputQueue::Remember(const InputCmdDraftV1& command) noexcept
{
    history_[history_cursor_].occupied = true;
    history_[history_cursor_].command = command;
    history_cursor_ = (history_cursor_ + 1) % history_.size();
    if (history_count_ < history_.size())
        ++history_count_;
}

AcceptResult InputQueue::Accept(const InputCmdDraftV1& command, std::uint64_t current_tick,
                                TickPhase phase) noexcept
{
    AcceptResult result{};
    result.depth_before = count_;
    result.depth_after = count_;
    if (retired_)
    {
        result.reason = RejectReason::SessionRetired;
        return result;
    }
    if (command.session_id != session_id_)
    {
        result.reason = RejectReason::WrongSession;
        return result;
    }
    if (command.client_id != client_id_)
    {
        result.reason = RejectReason::WrongClient;
        return result;
    }
    if (command.sequence == 0 || command.sequence == (std::numeric_limits<std::uint64_t>::max)())
    {
        result.reason = RejectReason::InvalidSequence;
        return result;
    }
    if (const HistoryEntry* previous = FindHistory(command.sequence))
    {
        if (SameCommand(previous->command, command))
        {
            result.decision = AcceptDecision::Duplicate;
            return result;
        }
        result.decision = AcceptDecision::FatalConflict;
        result.reason = RejectReason::SequenceConflict;
        retired_ = true;
        return result;
    }
    if (has_accepted_sequence_ && command.sequence < last_accepted_sequence_)
    {
        result.reason = RejectReason::StaleSequence;
        return result;
    }
    result.codec_error = ValidateInputState(command.state, valid_edge_mask_);
    if (result.codec_error != CodecError::None)
    {
        result.reason = RejectReason::InvalidCanonical;
        return result;
    }
    if (phase != TickPhase::DrainBeforeResolve || command.target_tick < current_tick)
    {
        result.reason = RejectReason::TargetExpired;
        return result;
    }
    if (command.target_tick - current_tick > kMaximumFutureTicks)
    {
        result.reason = RejectReason::TargetTooFar;
        return result;
    }
    for (std::uint32_t bit = 0; bit < 32; ++bit)
    {
        const std::uint32_t mask = 1u << bit;
        if ((command.state.edge_flags & mask) != 0 &&
            (edge_target_valid_mask_ & mask) != 0 && command.target_tick <= last_edge_target_[bit])
        {
            result.reason = RejectReason::EdgeTargetConflict;
            return result;
        }
    }
    if (count_ == commands_.size())
    {
        result.reason = RejectReason::QueueFull;
        return result;
    }

    result.sequence_gap = has_accepted_sequence_ ? command.sequence - last_accepted_sequence_ - 1
                                                 : command.sequence - 1;
    commands_[count_++] = command;
    Remember(command);
    for (std::uint32_t bit = 0; bit < 32; ++bit)
    {
        const std::uint32_t mask = 1u << bit;
        if ((command.state.edge_flags & mask) != 0)
        {
            last_edge_target_[bit] = command.target_tick;
            edge_target_valid_mask_ |= mask;
        }
    }
    last_accepted_sequence_ = command.sequence;
    has_accepted_sequence_ = true;
    result.decision = AcceptDecision::Accepted;
    result.depth_after = count_;
    return result;
}

ResolveResult InputQueue::Resolve(std::uint64_t tick) noexcept
{
    ResolveResult result{};
    if (retired_ || (has_resolved_tick_ && tick <= last_resolved_tick_))
        return result;
    result.resolved = true;
    std::size_t write = 0;
    bool have_continuous = false;
    std::uint64_t highest_sequence = 0;
    std::uint32_t combined_edges = 0;
    for (std::size_t read = 0; read < count_; ++read)
    {
        const InputCmdDraftV1& command = commands_[read];
        if (command.target_tick < tick)
        {
            ++result.expired_commands;
            continue;
        }
        if (command.target_tick == tick)
        {
            result.has_input = true;
            combined_edges |= command.state.edge_flags;
            result.effective.provenance_sequences[result.effective.provenance_count++] =
                command.sequence;
            if (!have_continuous || command.sequence > highest_sequence)
            {
                result.effective.state = command.state;
                highest_sequence = command.sequence;
                have_continuous = true;
            }
            continue;
        }
        if (write != read)
            commands_[write] = command;
        ++write;
    }
    count_ = write;
    if (result.has_input)
        result.effective.state.edge_flags = combined_edges;
    last_resolved_tick_ = tick;
    has_resolved_tick_ = true;
    return result;
}

void InputQueue::Retire() noexcept
{
    retired_ = true;
}

void InputQueue::Clear() noexcept
{
    count_ = 0;
    history_count_ = 0;
    history_cursor_ = 0;
    edge_target_valid_mask_ = 0;
    last_accepted_sequence_ = 0;
    last_resolved_tick_ = 0;
    has_accepted_sequence_ = false;
    has_resolved_tick_ = false;
    for (auto& entry : history_)
        entry.occupied = false;
    last_edge_target_.fill(0);
}

EdgePlanResult EdgeTargetPlanner::Plan(std::uint64_t observed_server_tick,
                                       const InputStateDraftV1& state,
                                       std::uint64_t& target_tick) noexcept
{
    if (ValidateInputState(state, valid_edge_mask_) != CodecError::None)
        return EdgePlanResult::InvalidCanonical;
    if (observed_server_tick >
        (std::numeric_limits<std::uint64_t>::max)() - kInputLeadTicks)
        return EdgePlanResult::TargetOverflow;
    std::uint64_t target = observed_server_tick + kInputLeadTicks;
    for (std::uint32_t bit = 0; bit < 32; ++bit)
    {
        const std::uint32_t mask = 1u << bit;
        if ((state.edge_flags & mask) == 0 || (edge_target_valid_mask_ & mask) == 0)
            continue;
        if (last_edge_target_[bit] == (std::numeric_limits<std::uint64_t>::max)())
            return EdgePlanResult::TargetOverflow;
        target = (std::max)(target, last_edge_target_[bit] + 1);
    }
    for (std::uint32_t bit = 0; bit < 32; ++bit)
    {
        const std::uint32_t mask = 1u << bit;
        if ((state.edge_flags & mask) != 0)
        {
            last_edge_target_[bit] = target;
            edge_target_valid_mask_ |= mask;
        }
    }
    target_tick = target;
    return EdgePlanResult::Planned;
}

void EdgeTargetPlanner::Reset() noexcept
{
    last_edge_target_.fill(0);
    edge_target_valid_mask_ = 0;
}

} // namespace isaac_cs_t0::input
